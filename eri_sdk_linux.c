#include <fcntl.h>
#include <unistd.h>

#include "eri_sdk.h"

i32 erisdk_open(const char *path) {
    i32 fd;
    fd = open(path, O_RDWR | O_CREAT);
    return fd;
}

u32 erisdk_write(i32 handle, const u8 *bytes, u32 size) {
    return (u32)write(handle, bytes, size);
}
