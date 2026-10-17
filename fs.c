#define WIN32_LEAN_AND_MEAN

#include "fs.h"

int fs_open(const char* path,
            file_handle* out) {
    HANDLE handle =
        CreateFileA(path,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN,
                    NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        return -1;
    }

    LARGE_INTEGER size;
    if (!GetFileSizeEx(handle, &size)) {
        CloseHandle(handle);
        return -1;
    }

    out->handle = handle;
    out->size = (long long)size.QuadPart;
    return 0;
}

void fs_close(file_handle* file) {
    CloseHandle(file->handle);
    file->handle = INVALID_HANDLE_VALUE;
    file->size = 0;
}
