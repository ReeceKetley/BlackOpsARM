"""Generate a validation-only original Windows T5 admission-protocol oracle."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess
from thread_data_layout_oracle import declaration
import verify_bootstrap_default_source as extraction

ROOT = Path(__file__).resolve().parents[2]
BASELINE = 'cfbd591'


def original(path):
    return subprocess.check_output(['git', 'show', BASELINE + ':' + path], cwd=ROOT).decode('utf-8-sig').replace('\r\n', '\n')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-directory', required=True)
    args = parser.parse_args()
    output = (ROOT / args.output_directory).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh worktree oracle directory')
    output.mkdir(parents=True)
    type_text = declaration(original('src/win32/win_common.h'), 'FastCriticalSection')
    bodies, provenance = [], []
    functions = [('src/win32/win_net.cpp', 'NET_Sleep', 'unsigned int msec'),
                 ('src/database/db_registry.cpp', 'Sys_LockWrite', 'FastCriticalSection *critSect'),
                 ('src/database/db_registry.cpp', 'Sys_UnlockWrite', 'FastCriticalSection *critSect'),
                 ('src/universal/dvar.cpp', 'Sys_UnlockRead', 'FastCriticalSection *critSect')]
    for path, name, parameters in functions:
        body = extraction.body(original(path), name)
        bodies.append(body)
        provenance.append({'path': path, 'function': name, 'body_sha256': hashlib.sha256(body.encode()).hexdigest()})
    source = '''#include <Windows.h>
#include <intrin.h>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <thread>
#include <vector>
bool Assert_MyHandler(const char *, int, int, const char *format, ...) {
    va_list args; va_start(args,format); std::vfprintf(stderr,format,args); va_end(args);
    std::fputc('\\n',stderr); std::abort();
}
''' + type_text + '\n' + '\n\n'.join(bodies) + '''
static_assert(sizeof(FastCriticalSection)==8 && alignof(FastCriticalSection)==4);
static_assert(offsetof(FastCriticalSection,readCount)==0 && offsetof(FastCriticalSection,writeCount)==4);
static void check(bool condition) { if(!condition) { std::fputs("FAIL original lock oracle\\n",stderr); std::_Exit(1); } }
static unsigned read(volatile long *value) { return static_cast<unsigned>(InterlockedCompareExchange(value,0,0)); }
static void contractReader(FastCriticalSection *lock) {
    // Original Dvar caller admission contract; no canonical Sys_LockRead exists.
    InterlockedExchangeAdd(&lock->readCount,1);
    while(read(&lock->writeCount)) NET_Sleep(0);
}
struct Guard { unsigned before=0x9e37ffee; FastCriticalSection lock{}; unsigned after=0xa15e1234; };
int main() {
    _set_abort_behavior(0,_WRITE_ABORT_MSG|_CALL_REPORTFAULT);
    std::thread([] { std::this_thread::sleep_for(std::chrono::seconds(20)); std::_Exit(2); }).detach();
    Guard guard;
    auto *lock=&guard.lock;
    contractReader(lock); contractReader(lock);
    std::atomic<bool> attempting{}, entered{}, finish{};
    std::thread writer([&] { attempting=true; Sys_LockWrite(lock); entered=true; while(!finish) std::this_thread::yield(); Sys_UnlockWrite(lock); });
    while(!attempting) std::this_thread::yield();
    check(read(&lock->readCount)==2 && !entered);
    Sys_UnlockRead(lock); check(read(&lock->readCount)==1 && !entered);
    Sys_UnlockRead(lock); while(!entered) std::this_thread::yield();
    std::atomic<bool> readerEntered{};
    std::thread reader([&] { contractReader(lock); readerEntered=true; Sys_UnlockRead(lock); });
    while(read(&lock->readCount)!=1) std::this_thread::yield();
    check(!readerEntered); finish=true; writer.join(); reader.join();
    check(readerEntered && !read(&lock->readCount) && !read(&lock->writeCount));
    unsigned generation=0, complement=~0u;
    std::atomic<bool> start{};
    std::atomic<unsigned> writing{};
    std::vector<std::thread> workers;
    for(unsigned i=0;i<3;++i) workers.emplace_back([&] {
        while(!start) std::this_thread::yield();
        for(unsigned j=0;j<1000;++j) { Sys_LockWrite(lock); check(writing.fetch_add(1)==0);
            ++generation; std::this_thread::yield(); complement=~generation;
            check(writing.fetch_sub(1)==1); Sys_UnlockWrite(lock); }
    });
    for(unsigned i=0;i<3;++i) workers.emplace_back([&] {
        while(!start) std::this_thread::yield();
        for(unsigned j=0;j<1000;++j) { contractReader(lock); check(generation==~complement && writing==0); Sys_UnlockRead(lock); }
    });
    start=true; for(auto &worker:workers) worker.join();
    check(generation==3000 && complement==~generation);
    check(!read(&lock->readCount) && !read(&lock->writeCount));
    check(guard.before==0x9e37ffee && guard.after==0xa15e1234);
    FastCriticalSection independent{}; Sys_LockWrite(lock); contractReader(&independent); Sys_UnlockRead(&independent); Sys_UnlockWrite(lock);
    std::printf("{\\"original_fast_critical_section\\":{\\"pointer_bytes\\":%zu,\\"lock_bytes\\":%zu,\\"writer_updates\\":3000,\\"reader_checks\\":3000,\\"queued_reader\\":true,\\"nested_readers\\":true,\\"canaries\\":true,\\"zero_final_counters\\":true,\\"passed\\":true}}\\n",sizeof(void*),sizeof(FastCriticalSection));
}
'''
    (output / 'original_fast_critical_section.cpp').write_text(source, encoding='utf-8', newline='\n')
    (output / 'oracle.json').write_text(json.dumps({'baseline': BASELINE,
            'canonical_type_sha256': hashlib.sha256(type_text.encode()).hexdigest(),
            'production_bodies': provenance, 'generated_cpp_sha256': hashlib.sha256(source.encode()).hexdigest(),
            'scope': 'Actual original writer/releases/NET_Sleep, reader admission contract driver; not Dvar_Init.'}, indent=2) + '\n', encoding='utf-8')
    print('Generated original Windows FastCriticalSection production-body oracle')


if __name__ == '__main__':
    main()
