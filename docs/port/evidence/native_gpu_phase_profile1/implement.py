from pathlib import Path
root=Path.cwd(); out=root/'docs/port/evidence/native_gpu_phase_profile1'
paths=['src/platform/android/performance_gpu_overlay.cpp','src/platform/android/performance_gpu_overlay.h','src/gfx_d3d/rb_backend.cpp']
for name in paths:
 p=root/name;(out/(p.name+'.before')).write_bytes(p.read_bytes())
p=root/paths[0]; raw=p.read_bytes(); s=raw.decode('cp1252'); newline='\r\n' if b'\r\n' in raw else '\n'; s=s.replace('\r\n','\n')
s=s.replace('#include <cerrno>','#include <cerrno>\n#if defined(BO1_NATIVE_GPU_PASS_PROFILE) && BO1_NATIVE_GPU_PASS_PROFILE\n#include <cstdio>\n#endif')
s=s.replace('constexpr unsigned SetCount = 8;','constexpr unsigned SetCount = 8;\n#if defined(BO1_NATIVE_GPU_PASS_PROFILE) && BO1_NATIVE_GPU_PASS_PROFILE\nconstexpr unsigned MarkCount = 7;\n#endif')
s=s.replace('    bool pending{}, eligible{};','    bool pending{}, eligible{};\n#if defined(BO1_NATIVE_GPU_PASS_PROFILE) && BO1_NATIVE_GPU_PASS_PROFILE\n    IDirect3DQuery9 *marks[MarkCount]{};\n    unsigned markMask{};\n#endif')
s=s.replace('    bool failed{};','    bool failed{};\n#if defined(BO1_NATIVE_GPU_PASS_PROFILE) && BO1_NATIVE_GPU_PASS_PROFILE\n    std::uint64_t lastPhaseReport{};\n#endif')
s=s.replace('        set = {};','''#if defined(BO1_NATIVE_GPU_PASS_PROFILE) && BO1_NATIVE_GPU_PASS_PROFILE
        for (auto *query : set.marks)
            if (query) query->Release();
#endif
        set = {};''',1)
s=s.replace('    owner.sequence = 0; owner.failed = false;','''    owner.sequence = 0; owner.failed = false;
#if defined(BO1_NATIVE_GPU_PASS_PROFILE) && BO1_NATIVE_GPU_PASS_PROFILE
    owner.lastPhaseReport = 0;
#endif''')
s=s.replace('    }\n    return true;\n}\nvoid Poll()', '''#if defined(BO1_NATIVE_GPU_PASS_PROFILE) && BO1_NATIVE_GPU_PASS_PROFILE
        for (auto &query : set.marks) {
            if (device->CreateQuery(D3DQUERYTYPE_TIMESTAMP, &query) != S_OK) {
                Fail(GpuStatus::Unsupported); return false;
            }
        }
#endif
    }
    return true;
}
#if defined(BO1_NATIVE_GPU_PASS_PROFILE) && BO1_NATIVE_GPU_PASS_PROFILE
void PublishPhases(const QuerySet &set, UINT64 frequency, UINT64 begin, UINT64 end, const UINT64 *marks) {
    if (owner.lastPhaseReport && (set.stamp < owner.lastPhaseReport
        || set.stamp - owner.lastPhaseReport < 1000000000ULL)) return;
    owner.lastPhaseReport = set.stamp;
    UINT64 previous = begin;
    bool monotonic = true;
    for (unsigned i = 0; i < MarkCount; ++i) {
        if (!(set.markMask & (1u << i))) continue;
        if (marks[i] < previous || marks[i] > end) monotonic = false;
        previous = marks[i];
    }
    char values[MarkCount + 1][32]{};
    for (unsigned i = 0; i <= MarkCount; ++i) {
        const bool haveStart = i == 0 || (set.markMask & (1u << (i - 1)));
        const bool haveEnd = i == MarkCount || (set.markMask & (1u << i));
        if (!monotonic || !haveStart || !haveEnd) {
            std::snprintf(values[i], sizeof(values[i]), "n/a");
        } else {
            const UINT64 start = i == 0 ? begin : marks[i - 1];
            const UINT64 finish = i == MarkCount ? end : marks[i];
            std::snprintf(values[i], sizeof(values[i]), "%.3f",
                static_cast<double>(finish - start) * 1000.0 / static_cast<double>(frequency));
        }
    }
    std::fprintf(stderr, "BO1_NATIVE_GPU_PHASE seq=%llu total_ms=%.3f mask=0x%02x phase_valid=%d p0_ms=%s p1_ms=%s p2_ms=%s p3_ms=%s p4_ms=%s p5_ms=%s p6_ms=%s p7_ms=%s\\n",
        static_cast<unsigned long long>(set.sequence),
        static_cast<double>(end - begin) * 1000.0 / static_cast<double>(frequency),
        set.markMask, monotonic ? 1 : 0, values[0], values[1], values[2], values[3],
        values[4], values[5], values[6], values[7]);
}
#endif
void Poll()''')
s=s.replace('        if (pending) { GpuObservation', '''#if defined(BO1_NATIVE_GPU_PASS_PROFILE) && BO1_NATIVE_GPU_PASS_PROFILE
        UINT64 marks[MarkCount]{};
        for (unsigned i = 0; i < MarkCount; ++i) {
            if (!(oldest->markMask & (1u << i))) continue;
            const HRESULT result = oldest->marks[i]->GetData(&marks[i], sizeof(marks[i]), 0);
            if (result == S_FALSE) pending = true;
            else if (result != S_OK) { Fail(GpuStatus::Failed); return; }
        }
#endif
        if (pending) { GpuObservation''')
s=s.replace('            GpuObservation(owner.epoch, GpuStatus::Valid, ms, oldest->stamp);','''            GpuObservation(owner.epoch, GpuStatus::Valid, ms, oldest->stamp);
#if defined(BO1_NATIVE_GPU_PASS_PROFILE) && BO1_NATIVE_GPU_PASS_PROFILE
            PublishPhases(*oldest, frequency, begin, end, marks);
#endif''')
s=s.replace('        set.sequence = ++owner.sequence;', '''#if defined(BO1_NATIVE_GPU_PASS_PROFILE) && BO1_NATIVE_GPU_PASS_PROFILE
        set.markMask = 0;
#endif
        set.sequence = ++owner.sequence;''')
s=s.replace('void BeforePresent() {','''void Mark(int index) {
#if defined(BO1_NATIVE_GPU_PASS_PROFILE) && BO1_NATIVE_GPU_PASS_PROFILE
    if (index < 0 || index >= static_cast<int>(MarkCount)) return;
    const unsigned epoch = Epoch();
    std::lock_guard<std::mutex> lock(owner.mutex);
    if (!epoch || epoch != owner.epoch || owner.failed || owner.open < 0) return;
    auto &set = owner.sets[owner.open];
    const unsigned bit = 1u << static_cast<unsigned>(index);
    if (set.pending || set.eligible || (set.markMask & bit)) return;
    if (set.marks[index]->Issue(D3DISSUE_END) != S_OK) {
        Fail(GpuStatus::Failed); return;
    }
    set.markMask |= bit;
#else
    (void)index;
#endif
}
void BeforePresent() {''')
p.write_bytes(s.replace('\n',newline).encode('cp1252'))
p=root/paths[1]; raw=p.read_bytes(); s=raw.decode('cp1252'); nl='\r\n' if b'\r\n' in raw else '\n';s=s.replace('void BeforePresent();','// Optional bounded phase profiling; disabled builds perform no query work.'+nl+'void Mark(int index);'+nl+'void BeforePresent();');p.write_bytes(s.encode('cp1252'))
p=root/paths[2];raw=p.read_bytes();s=raw.decode('cp1252');nl='\r\n' if b'\r\n' in raw else '\n';needle='void RB_BO1GpuMark(int idx)'+nl+'{'; assert s.count(needle)==1;s=s.replace(needle,needle+nl+'#if defined(__ANDROID__)'+nl+'    bo1::android::performance::gpu::Mark(idx);'+nl+'#endif',1);p.write_bytes(s.encode('cp1252'))
