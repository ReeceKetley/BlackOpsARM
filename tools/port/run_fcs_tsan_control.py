"""Separate Android sanitizer runtime behavior from T5 lock execution."""
from pathlib import Path
import argparse
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', required=True)
    parser.add_argument('--serial', required=True)
    args = parser.parse_args()
    output = (ROOT / args.output).resolve()
    if not output.is_relative_to(ROOT) or output.exists():
        raise RuntimeError('Use fresh worktree control output')
    output.mkdir(parents=True)
    source = output / 'sanitizer_control.cpp'
    source.write_text('''#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
int main() {
    std::puts("SANITIZER_CONTROL_MAIN_ENTER"); std::fflush(stdout);
    std::thread([] { std::this_thread::sleep_for(std::chrono::seconds(20)); std::_Exit(2); }).detach();
    std::atomic<unsigned> count{};
    std::thread worker([&] { for(unsigned i=0;i<1000;++i) count.fetch_add(1); });
    worker.join();
    if(count!=1000) return 1;
    std::puts("SANITIZER_CONTROL_THREADS_PASS"); std::fflush(stdout);
}
''', encoding='utf-8', newline='\n')
    ndk = Path('D:/Android/Sdk/ndk/30.0.16248370/toolchains/llvm/prebuilt/windows-x86_64')
    runtime = ndk / 'lib/clang/21/lib/linux/libclang_rt.tsan-aarch64-android.so'
    adb = ['D:/Android/Sdk/platform-tools/adb.exe', '-s', args.serial]
    commands, rows = [], []

    def run(arguments, zero=True):
        completed = subprocess.run([str(a) for a in arguments], capture_output=True, text=True, timeout=30)
        commands.append({'args': [str(a) for a in arguments], 'exit': completed.returncode,
                         'stdout': completed.stdout, 'stderr': completed.stderr})
        (output / 'commands.json').write_text(json.dumps(commands, indent=2) + '\n', encoding='utf-8')
        if zero and completed.returncode:
            raise RuntimeError('Sanitizer control setup failed')
        return completed

    run(adb + ['get-state'])
    for label, sanitizer in [('thread_only', 'thread'), ('thread_undefined', 'thread,undefined')]:
        binary = output / label
        run([ndk / 'bin/clang++.exe', '--target=aarch64-none-linux-android28',
             '--sysroot=' + str(ndk / 'sysroot'), '-std=c++20', '-O1', '-g',
             '-static-libstdc++', '-fsanitize=' + sanitizer, source, '-o', binary])
        remote = '/data/local/tmp/blackopsarm-fcs45-tsan-control-' + label
        run(adb + ['shell', 'mkdir', '-p', remote])
        for path, name in [(binary, 'control'), (runtime, runtime.name)]:
            run(adb + ['push', path, remote + '/' + name])
            if run(adb + ['shell', 'sha256sum', remote + '/' + name]).stdout.split()[0] != hashlib.sha256(path.read_bytes()).hexdigest():
                raise RuntimeError('Sanitizer control transfer differs')
        run(adb + ['shell', 'chmod', '700', remote + '/control'])
        completed = run(adb + ['shell', 'env', 'LD_LIBRARY_PATH=' + remote,
                        'TSAN_OPTIONS=halt_on_error=1:exitcode=66:memory_limit_mb=256', remote + '/control'], zero=False)
        rows.append({'label': label, 'exit': completed.returncode, 'stdout': completed.stdout,
                     'stderr': completed.stderr, 'binary_sha256': hashlib.sha256(binary.read_bytes()).hexdigest()})
    (output / 'result.json').write_text(json.dumps({'control_runs': rows,
            'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
            'runtime_sha256': hashlib.sha256(runtime.read_bytes()).hexdigest(),
            'scope': 'No engine code; only native atomic counter and std::thread with watchdog.'}, indent=2) + '\n', encoding='utf-8')
    print('Sanitizer controls recorded independently of engine code')


if __name__ == '__main__':
    main()
