#include <platform/win32_compat/win32_errors.h>
#include <cerrno>
#include <unistd.h>

int DeleteFileA(const char *path) {
    if (!path) {
        bo1::win32::SetLastError(87); // ERROR_INVALID_PARAMETER
        return 0;
    }
    if (unlink(path) == 0) return 1;
    bo1::win32::SetLastError(errno == ENOENT ? 2 : errno == EACCES ? 5 : 31);
    return 0;
}
