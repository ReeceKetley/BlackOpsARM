#pragma once

// Portable filesystem surface. Windows backend forwards to existing FS_* code.
// Callers should migrate to this gradually; stock paths still use FS_* directly.

struct IFileSystem
{
    virtual ~IFileSystem() = default;

    virtual const char *Name() const = 0;
    // Returns file size, or -1 on failure. *outBuffer is engine-owned; FreeFile releases it.
    virtual int ReadFile(const char *qpath, void **outBuffer) = 0;
    virtual void FreeFile(void *buffer) = 0;
    virtual int WriteFile(const char *filename, const void *buffer, unsigned int size) = 0;
};
