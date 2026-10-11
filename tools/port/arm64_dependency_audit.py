"""Reproducible lexical dependency inventory; findings are candidates, not bugs."""
import argparse
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
CATEGORIES = {
    "Windows headers": {
        "OS SDK headers": r'#\s*include\s*[<"](?:windows\.h|winbase\.h|winnt\.h|windef\.h|winsock2?\.h|ws2tcpip\.h|process\.h|mmsystem\.h|dbghelp\.h|psapi\.h|winioctl\.h)[>"]',
    },
    "Windows API types/constants": {
        "LLP64 scalar and opaque types": r'\b(?:DWORD|BOOL|LONG|ULONG|HANDLE|HMODULE|HWND|LPVOID|LARGE_INTEGER|_LARGE_INTEGER|CRITICAL_SECTION|SRWLOCK|CONTEXT)\b',
        "OS flags and error contracts": r'\b(?:INVALID_HANDLE_VALUE|INFINITE|WAIT_OBJECT_0|WAIT_TIMEOUT|ERROR_[A-Z_]+|MEM_[A-Z_]+|PAGE_[A-Z_]+)\b',
    },
    "Memory allocation/virtual memory": {
        "reserve/commit/protect/query": r'\b(?:VirtualAlloc|VirtualFree|VirtualProtect|VirtualQuery|VirtualAllocEx)\s*\(',
        "heap/aligned allocation ownership": r'\b(?:HeapAlloc|HeapFree|HeapCreate|GlobalAlloc|LocalAlloc|_aligned_malloc|_aligned_free|_aligned_realloc)\s*\(',
    },
    "Files/filesystem paths": {
        "handle-based file and async IO": r'\b(?:CreateFile[AW]?|ReadFile|WriteFile|SetFilePointer(?:Ex)?|GetFileSize(?:Ex)?|FlushFileBuffers|OVERLAPPED|FILE_FLAG_[A-Z_]+)\b',
        "path/enumeration/CRT differences": r'\b(?:FindFirstFile[AW]?|FindNextFile[AW]?|GetFileAttributes[AW]?|CreateDirectory[AW]?|GetCurrentDirectory[AW]?|_findfirst|_findnext|_fseeki64|_ftelli64|PATH_SEP|MAX_PATH)\b',
    },
    "Threads and synchronization": {
        "thread lifecycle/identity": r'\b(?:CreateThread|_beginthreadex|ResumeThread|SuspendThread|GetCurrentThreadId|SetThreadAffinityMask|SetThreadPriority|TerminateThread)\s*\(',
        "events/waits/locks": r'\b(?:CreateEvent[AW]?|SetEvent|ResetEvent|WaitForSingleObject|WaitForMultipleObjects|EnterCriticalSection|LeaveCriticalSection|InitializeCriticalSection|AcquireSRWLockExclusive|ReleaseSRWLockExclusive)\s*\(',
        "atomic publication/order": r'\b(?:_?Interlocked\w*|MemoryBarrier|volatile)\b',
    },
    "Timing/performance counters": {
        "clock units and scheduling": r'\b(?:QueryPerformanceCounter|QueryPerformanceFrequency|GetTickCount(?:64)?|timeGetTime|Sleep)\s*\(',
        "CPU tick assumptions": r'\b(?:__rdtsc|__rdtscp|__readfsdword)\s*\(',
    },
    "Thread-local storage": {
        "Windows TLS lifecycle": r'\b(?:TlsAlloc|TlsFree|TlsGetValue|TlsSetValue|FlsAlloc|FlsGetValue|FlsSetValue)\s*\(',
        "compiler TLS/error state": r'\bthread_local\b|__declspec\s*\(\s*thread\s*\)|\b(?:GetLastError|SetLastError)\s*\(',
    },
    "MSVC extensions/intrinsics": {
        "syntax and fixed-width spellings": r'\b(?:__int8|__int16|__int32|__int64|__forceinline|__declspec|__interface|__uuidof)\b',
        "compiler helper intrinsics": r'\b(?:__debugbreak|_BitScan\w*|_byteswap_\w*|_rotl\w*|__assume|__fastfail)\b|#\s*include\s*[<"]intrin\.h[>"]',
        "MSVC CRT helpers": r'\b(?:_stricmp|_strnicmp|_snprintf|_vsnprintf|_atoi64|fopen_s|strcpy_s|sprintf_s)\s*\(',
    },
    "x86/x64 assembly and SIMD": {
        "x86 SIMD instruction types": r'\b(?:__m128\w*|__m256\w*|_mm_\w*|_mm256_\w*)\b|#\s*include\s*[<"](?:xmmintrin|emmintrin|immintrin|mmintrin)\.h[>"]',
        "assembly/register/architecture assumptions": r'\b(?:__asm|_M_IX86|_M_X64|__readfsdword|__readgsqword)\b|\.(?:Eip|Rip|Esp|Rsp|Ebp|Rbp)\b',
    },
    "ABI/calling conventions": {
        "calling conventions and varargs": r'\b(?:__cdecl|__stdcall|__fastcall|__thiscall|WINAPI|CALLBACK|va_list)\b',
        "native long/wide scalar assumptions": r'\b(?:unsigned\s+long|signed\s+long|wchar_t|long\s+double|sint32)\b',
        "Windows-only 64-bit gates": r'\b(?:BO1_PORT_X64|_WIN64|BO1_ARCH_64)\b',
    },
    "Pointer/integer conversions": {
        "narrow pointer/handle encoding": r'\b(?:Narrow32|BO1_NARROW\w*|Bo1_Narrow\w*)\b',
        "cast candidates needing type review": r'reinterpret_cast\s*<\s*(?:u?intptr_t|unsigned\s+int|int|u?int(?:32|64)_t)|\(\s*(?:unsigned\s+int|int|uint32_t|DWORD)\s*\)\s*[A-Za-z_][\w.>\[\]-]*',
    },
    "Structure packing/alignment": {
        "explicit layout controls": r'#\s*pragma\s+pack|\balignas\s*\(|__declspec\s*\(\s*align|__attribute__\s*\(\(\s*(?:packed|aligned)',
        "layout checks and stream expansion": r'\b(?:static_assert|offsetof|DB_Expand\w*|Ptr32Array|Disk\w*32)\b',
        "aliasing/bit-punning helpers": r'\b(?:COERCE_\w+|LODWORD|HIDWORD|LOWORD|HIWORD|BYTE[1-7]|WORD[1-3])\b',
    },
    "Direct3D 9/D3DX": {
        "D3D device/COM/layout dependency": r'\b(?:IDirect3D\w*|D3D\w*|Direct3DCreate9\w*)\b|#\s*include\s*[<"]d3d9[^>"]*[>"]',
        "D3DX compiler/reflection/helpers": r'\b(?:ID3DX\w*|D3DX\w*)\b|#\s*include\s*[<"]d3dx[^>"]*[>"]',
    },
    "Windows audio": {
        "XAudio2/XAPO device and callback ABI": r'\b(?:IXAudio2\w*|XAUDIO2\w*|XAudio2Create|XAPO\w*|IXAPO\w*)\b',
        "DirectSound/MMDevice/wave APIs": r'\b(?:IDirectSound\w*|DirectSound\w*|IMMDevice\w*|IAudioClient\w*|waveOut\w*|WAVEFORMAT\w*)\b',
    },
}
VENDOR = ("src/libs/", "src/steam/", "src/tracy/", "src/speex/", "src/zlib/",
          "src/jpeg/", "src/minilzo/", "src/nvapi/", "src/binklib/", "src/vpx/",
          "src/mjpeg/")
COMMENTS = re.compile(r'"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|//[^\n]*|/\*.*?\*/', re.S)


def without_comments(text):
    return COMMENTS.sub(lambda m: ("\n" * m[0].count("\n")) if m[0].startswith(("//", "/*")) else m[0], text)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    tracked = subprocess.check_output(["git", "ls-files", "src", "tl", "DemonWare"], cwd=ROOT, text=True).splitlines()
    candidates = [p for p in tracked if Path(p).suffix.lower() in {".h", ".hpp", ".c", ".cpp", ".inl"}]
    files = [p for p in candidates if not p.startswith(VENDOR)]
    texts = {p: without_comments((ROOT / p).read_text(encoding="utf-8-sig", errors="replace")) for p in files}
    report = {"scope": "Git-tracked engine src/tl/DemonWare C/C++ excluding listed vendor trees; comments removed, strings and inactive branches retained",
              "interpretation": "Lexical candidates; unique root causes are named pattern groups, not exhaustive semantic bug counts; categories overlap",
              "filesScanned": len(files), "vendorFilesExcluded": len(candidates) - len(files), "excludedPrefixes": VENDOR,
              "categories": {}}
    for category, roots in CATEGORIES.items():
        root_reports, affected = {}, set()
        for name, pattern in roots.items():
            regex = re.compile(pattern, re.I if category == "Windows headers" else 0)
            locations = []
            root_files = set()
            for path, content in texts.items():
                for match in regex.finditer(content):
                    root_files.add(path)
                    locations.append({"file": path, "line": content.count("\n", 0, match.start()) + 1, "match": match[0]})
            affected.update(root_files)
            root_reports[name] = {"affectedFiles": len(root_files), "occurrences": len(locations),
                                  "representatives": locations[:6], "files": sorted(root_files)}
        report["categories"][category] = {"uniqueRootCauses": sum(bool(v["affectedFiles"]) for v in root_reports.values()),
                                           "affectedFiles": len(affected), "roots": root_reports}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"Scanned {len(files)} engine files; excluded {len(candidates) - len(files)} vendor files.")
    for name, item in report["categories"].items():
        print(f"{name}: {item['uniqueRootCauses']} root groups, {item['affectedFiles']} candidate files")


if __name__ == "__main__":
    main()
