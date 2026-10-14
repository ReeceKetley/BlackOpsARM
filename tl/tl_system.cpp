#include "tl_system.h"
#include <stdio.h>
#include <cstdio>


#include <cstring>
#if defined(__ANDROID__)
#include <platform/android/tl_memory.h>
#include <platform/android/file_services.h>
#include <platform/android/legacy_crt.h>
#include <platform/win32_compat/win32_time.h>
#include <atomic>
#include <algorithm>
#include <mutex>
#include <limits>
#include <string>
#include <sys/resource.h>
#include <unistd.h>
static void OutputDebugStringA(const char *text) {std::fputs(text,stderr);std::fflush(stderr);}
using bo1::win32::QueryPerformanceCounter;
#else
#include <corecrt_malloc.h>
#include <Windows.h>
#include <Psapi.h>
#endif

tlSystemCallbacks tlCurSystemCallbacks;
char tlHostPrefix[256];
#if defined(__ANDROID__)
std::atomic<int> tlMemAllocCounter;
#else
int tlMemAllocCounter;
#endif
int tlScratchPadRefCount;
bool tlScratchpadLocked;
void *tlScratchPadPtr;

void *tlStackBegin;
void *tlStackEnd;

#if defined(__ANDROID__)
float tlPcTicksPerMS = 1000000.0f;
#else
float tlPcTicksPerMS = 1.0f;
#endif

void __cdecl tlSetSystemCallbacks(const tlSystemCallbacks *Callbacks)
{
  memcpy(&tlCurSystemCallbacks, Callbacks, sizeof(tlCurSystemCallbacks));
}

void __cdecl tlSetFileServerRootPC(const char *Path)
{
  strcpy(tlHostPrefix, Path);
}

void __cdecl tlMemFree(void *Ptr)
{
#if defined(__ANDROID__)
  if(Ptr){bo1::android::TLFree(Ptr);--tlMemAllocCounter;}
#else
  --tlMemAllocCounter;
  if ( tlCurSystemCallbacks.MemFree )
    tlCurSystemCallbacks.MemFree(Ptr);
  else
    _aligned_free(Ptr);
#endif
}

void __cdecl tlReleaseFile(tlFileBuf *File)
{
  unsigned __int8 *Buf; // eax

  if ( tlCurSystemCallbacks.ReleaseFile )
  {
    tlCurSystemCallbacks.ReleaseFile(File);
  }
  else
  {
    Buf = File->Buf;
#if defined(__ANDROID__)
    tlMemFree(Buf);
#else
    --tlMemAllocCounter;
    if ( tlCurSystemCallbacks.MemFree )
      tlCurSystemCallbacks.MemFree(Buf);
    else
      _aligned_free(Buf);
#endif
    File->Buf = 0;
    File->Size = 0;
    File->UserData = 0;
  }
}

BO1_TL_COUNTER __cdecl tlPcGetTick()
{
  BO1_TL_COUNTER li; // [esp+0h] [ebp-8h] BYREF

  QueryPerformanceCounter(&li);
  return li;
}

void __cdecl tlPrint(const char *txt)
{
  OutputDebugStringA(txt);
}

int __cdecl tlGetVersion()
{
  return 66560;
}

void __cdecl tlStackRangeInit()
{
  tlStackBegin = 0;
  tlStackEnd = 0;
}

char __cdecl tlFatalHandler(const char *Msg)
{
  if ( tlCurSystemCallbacks.CriticalError )
  {
    tlCurSystemCallbacks.CriticalError(Msg);
    return 0;
  }
  else
  {
    OutputDebugStringA("TL Fatal Error: ");
    OutputDebugStringA(Msg);
    OutputDebugStringA("\n");
    return 1;
  }
}

void __cdecl tlDebugPrint(const char *txt)
{
  if ( tlCurSystemCallbacks.DebugPrint )
    tlCurSystemCallbacks.DebugPrint(txt);
  else
    OutputDebugStringA(txt);
}

void __cdecl tlVPrintf(const char *Format, va_list args)
{
  char Work[512]; // [esp+0h] [ebp-204h] BYREF

  std::vsnprintf(Work, sizeof(Work), Format, args);
  if ( tlCurSystemCallbacks.DebugPrint )
    tlCurSystemCallbacks.DebugPrint(Work);
  else
    OutputDebugStringA(Work);
}

void tlPrintf(const char *Format, ...)
{
  char Work[512]; // [esp+0h] [ebp-204h] BYREF
  va_list ap; // [esp+210h] [ebp+Ch] BYREF

  va_start(ap, Format);
  std::vsnprintf(Work, sizeof(Work), Format, ap);
  va_end(ap);
  if ( tlCurSystemCallbacks.DebugPrint )
    tlCurSystemCallbacks.DebugPrint(Work);
  else
    OutputDebugStringA(Work);
}

char __cdecl _tlAssert(const char *file, int line, const char *expr, const char *desc)
{
  char Buf[256]; // [esp+0h] [ebp-104h] BYREF

  _snprintf(Buf, 0x100u, "ASSERT in %s(%d):\n\"%s\" - %s", file, line, expr, desc);
  Buf[255] = 0;
  if ( tlCurSystemCallbacks.CriticalError )
  {
    tlCurSystemCallbacks.CriticalError(Buf);
    return 0;
  }
  else
  {
    OutputDebugStringA("TL Fatal Error: ");
    OutputDebugStringA(Buf);
    OutputDebugStringA("\n");
    return 1;
  }
}

void tlFatal(const char *Format, ...)
{
  char Work[512]; // [esp+0h] [ebp-204h] BYREF
  va_list ap; // [esp+210h] [ebp+Ch] BYREF

  va_start(ap, Format);
  std::vsnprintf(Work, sizeof(Work), Format, ap);
  va_end(ap);
  if ( tlCurSystemCallbacks.CriticalError )
  {
    tlCurSystemCallbacks.CriticalError(Work);
  }
  else
  {
    OutputDebugStringA("TL Fatal Error: ");
    OutputDebugStringA(Work);
    OutputDebugStringA("\n");
    __debugbreak();
  }
}

void *__cdecl tlMemAlloc(unsigned int Size, unsigned int Align, unsigned int Flags)
{
#if defined(__ANDROID__)
  void *memory=bo1::android::TLAllocate(Size,Align,Flags,tlCurSystemCallbacks);
  if(memory)++tlMemAllocCounter;
  else if(!(Flags&2) && Size)tlFatal("Memory allocation failed. %u bytes, %u align",Size,Align);
  return memory;
#else
  unsigned int v3; // edi
  void *v4; // eax
  void *v5; // esi

  v3 = Align;
  if ( !Align && (Size & 0xF) == 0 )
    v3 = 0;
  ++tlMemAllocCounter;
  if ( tlCurSystemCallbacks.MemAlloc )
    v4 = tlCurSystemCallbacks.MemAlloc(Size, v3, Flags);
  else
    v4 = _aligned_malloc(Size, v3);
  v5 = v4;
  if ( (Flags & 2) == 0 && !v4 && Size )
    tlFatal("Memory allocation failed. %d bytes, %d align", Size, v3);
  return v5;
#endif
}

void *__cdecl tlMemRealloc(void *Ptr, unsigned int Size, unsigned int Align, unsigned int Flags)
{
#if defined(__ANDROID__)
  if(!Ptr)return tlMemAlloc(Size,Align,Flags);
  if(!Size){tlMemFree(Ptr);return nullptr;}
  void *memory=bo1::android::TLReallocate(Ptr,Size,Align,Flags);
  if(!memory && !(Flags&2))tlFatal("Memory reallocation failed.");
  return memory;
#else
  unsigned int v4; // eax
  void *v5; // eax
  void *v6; // esi

  v4 = Align;
  if ( !Align && (Size & 0xF) == 0 )
    v4 = 0;
  if ( tlCurSystemCallbacks.MemRealloc )
    v5 = tlCurSystemCallbacks.MemRealloc(Ptr, Size, v4, Flags);
  else
    v5 = _aligned_realloc(Ptr, Size, v4);
  v6 = v5;
  if ( (Flags & 2) == 0 && !v5 && Size )
    tlFatal("Memory reallocation failed.");
  return v6;
#endif
}

unsigned int __cdecl tlGetFreeMemory()
{
#if defined(__ANDROID__)
  const long pages=sysconf(_SC_PHYS_PAGES),pageBytes=sysconf(_SC_PAGESIZE);
  rusage usage{};
  if(pages<=0 || pageBytes<=0 || getrusage(RUSAGE_SELF,&usage) || usage.ru_maxrss<0)return 0;
  const auto total=static_cast<std::uint64_t>(pages)*pageBytes;
  const auto peak=static_cast<std::uint64_t>(usage.ru_maxrss)*1024;
  const auto remaining=peak<total?total-peak:0;
  return static_cast<unsigned>(std::min<std::uint64_t>(remaining,std::numeric_limits<unsigned>::max()));
#else
  unsigned int v0; // edi
  unsigned int dwTotalPhys; // ebx
  DWORD CurrentProcessId; // eax
  HANDLE v3; // esi
  _PROCESS_MEMORY_COUNTERS pmc; // [esp+Ch] [ebp-48h] BYREF
  _MEMORYSTATUS stat; // [esp+34h] [ebp-20h] BYREF

  v0 = 0;
  GlobalMemoryStatus(&stat);
  dwTotalPhys = stat.dwTotalPhys;
  GetCurrentProcess();
  CurrentProcessId = GetCurrentProcessId();
  v3 = OpenProcess(0x410u, 0, CurrentProcessId);
  if ( GetProcessMemoryInfo(v3, &pmc, 40) )
    v0 = dwTotalPhys - pmc.PeakWorkingSetSize;
  CloseHandle(v3);
  return v0;
#endif
}

void *__cdecl tlScratchPadInit()
{
#if defined(__ANDROID__)
  extern std::mutex &tlScratchMutex();
  std::lock_guard<std::mutex> lock(tlScratchMutex());
  if(tlScratchPadRefCount++ == 0)tlScratchPadPtr=tlMemAlloc(0x4000,16,0);
  return tlScratchPadPtr;
#else
  void *v2; // eax
  void *v3; // esi

  if ( tlScratchPadRefCount++ )
    return tlScratchPadPtr;
  ++tlMemAllocCounter;
  if ( tlCurSystemCallbacks.MemAlloc )
    v2 = tlCurSystemCallbacks.MemAlloc(0x4000u, 0x10u, 0);
  else
    v2 = _aligned_malloc(0x4000u, 0x10u);
  v3 = v2;
  if ( !v2 )
    tlFatal("Memory allocation failed. %d bytes, %d align", 0x4000, 16);
  tlScratchPadPtr = v3;
  return v3;
#endif
}

void __cdecl tlScratchPadReset()
{
#if defined(__ANDROID__)
  extern std::mutex &tlScratchMutex();
  std::lock_guard<std::mutex> lock(tlScratchMutex());
#endif
  if ( tlScratchPadRefCount < 1
    && _tlAssert("source/tl_system.cpp", 830, "tlScratchPadRefCount >= 1", "Scratchpad reset underflow.") )
  {
    __debugbreak();
  }
  if ( !--tlScratchPadRefCount )
  {
#if defined(__ANDROID__)
    tlMemFree(tlScratchPadPtr);
#else
    --tlMemAllocCounter;
    if ( tlCurSystemCallbacks.MemFree )
      tlCurSystemCallbacks.MemFree(tlScratchPadPtr);
    else
      _aligned_free(tlScratchPadPtr);
#endif
    tlScratchPadPtr = 0;
  }
}

void tlWarning(const char *Format, ...)
{
  char Work[512]; // [esp+0h] [ebp-204h] BYREF
  va_list ap; // [esp+210h] [ebp+Ch] BYREF

  va_start(ap, Format);
  std::vsnprintf(Work, sizeof(Work), Format, ap);
  va_end(ap);
  if ( tlCurSystemCallbacks.Warning )
    tlCurSystemCallbacks.Warning(Work);
  else
    tlPrintf("%s", Work);
}

bool __cdecl tlReadFile(const char *FileName, tlFileBuf *File, unsigned int Align, unsigned int Flags)
{
#if defined(__ANDROID__)
  if(!FileName || !File)return false;
  if(tlCurSystemCallbacks.ReadFile)return tlCurSystemCallbacks.ReadFile(FileName,File,Align,Flags);
  std::string path;
  if(std::strncmp(FileName,tlHostPrefix,std::strlen(tlHostPrefix)))path=tlHostPrefix;
  path+=FileName;
  if(path.size()>=512)return false;
  auto handle=CreateFileA(path.c_str(),0x80000000u,1,nullptr,3,0x08000080u,nullptr);
  if(handle==reinterpret_cast<void *>(std::intptr_t(-1)))return false;
  std::uint32_t high=0,size=GetFileSize(handle,&high);
  if(high || (size==0xffffffffu && GetLastError())){CloseHandle(handle);return false;}
  auto *buffer=static_cast<unsigned char *>(tlMemAlloc(size,Align,Flags));
  if(!buffer){CloseHandle(handle);return false;}
  std::uint32_t done=0;
  while(done<size) {
    std::uint32_t count=0;
    if(!ReadFile(handle,buffer+done,size-done,&count,nullptr) || !count){tlMemFree(buffer);CloseHandle(handle);return false;}
    done+=count;
  }
  if(!CloseHandle(handle)){tlMemFree(buffer);return false;}
  File->Buf=buffer;File->Size=size;File->UserData=0;return true;
#else
  int v5; // eax
  char v6; // cl
  unsigned int v7; // eax
  char *v8; // edi
  char *i; // eax
  HANDLE FileA; // eax
  void *v12; // esi
  unsigned int FileSize; // eax
  unsigned __int8 *v14; // eax
  unsigned int Size; // [esp-Ch] [ebp-21Ch]
  DWORD bytesRead; // [esp+8h] [ebp-208h] BYREF
  char Work[512]; // [esp+Ch] [ebp-204h] BYREF

  if ( tlCurSystemCallbacks.ReadFile )
    return tlCurSystemCallbacks.ReadFile(FileName, File, Align, Flags);
  if ( !strncmp(FileName, tlHostPrefix, strlen(tlHostPrefix)) )
  {
    Work[0] = 0;
  }
  else
  {
    v5 = 0;
    do
    {
      v6 = tlHostPrefix[v5];
      Work[v5++] = v6;
    }
    while ( v6 );
  }
  v7 = strlen(FileName) + 1;
  v8 = (char *)&bytesRead + 3;
  while ( *++v8 )
    ;
  memcpy(v8, FileName, v7);
  for ( i = Work; *i; ++i )
  {
    if ( *i == 47 )
      *i = 92;
  }
  FileA = CreateFileA(Work, 0x80000000, 1u, 0, 3u, 0x8000080u, 0);
  v12 = FileA;
  if ( FileA == (HANDLE)-1 )
    return 0;
  FileSize = GetFileSize(FileA, 0);
  File->Size = FileSize;
  v14 = (unsigned __int8 *)tlMemAlloc(FileSize, Align, Flags);
  Size = File->Size;
  File->Buf = v14;
  ReadFile(v12, v14, Size, &bytesRead, 0);
  CloseHandle(v12);
  return 1;
#endif
}
#if defined(__ANDROID__)
std::mutex &tlScratchMutex(){static auto *value=new std::mutex;return *value;}
#endif

