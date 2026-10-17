#ifndef FS_H
#define FS_H

#include <windows.h>

typedef struct {
    HANDLE handle;
    long long size;
} file_handle;

int fs_open(const char* path,
            file_handle* out);
void fs_close(file_handle* file);

#endif
