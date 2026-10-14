#pragma once
#include <cstdint>
#include <string>
#include <platform/win32_compat/async_file_types.h>
void *CreateFileA(const char *path, std::uint32_t access, std::uint32_t sharing,
                  void *security, std::uint32_t creation, std::uint32_t flags, void *templateFile);
int ReadFile(void *handle, void *buffer, std::uint32_t bytes, std::uint32_t *read, OVERLAPPED *overlapped);
int WriteFile(void *handle, const void *buffer, std::uint32_t bytes, std::uint32_t *written, OVERLAPPED *overlapped);
int CloseHandle(void *handle);
std::uint32_t GetFileSize(void *handle, std::uint32_t *high);
std::uint32_t GetFileAttributesA(const char *path);
int SetFileAttributesA(const char *path,std::uint32_t attributes);
std::uint32_t GetLastError();
using BO1_FileCompletion = void (*)(std::uint32_t, std::uint32_t, OVERLAPPED *);
int ReadFileEx(void *handle, void *buffer, std::uint32_t bytes,
               OVERLAPPED *overlapped, BO1_FileCompletion completion);
std::uint32_t SleepEx(std::uint32_t milliseconds, int alertable);
namespace bo1::android { std::string ResolveEngineFilePath(const char *path); }
void Sys_Mkdir(const char *path);
int Sys_DirectoryHasContents(const char *directory);
