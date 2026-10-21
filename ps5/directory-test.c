#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>

DIR* ps5_native_opendir(const char*);
struct dirent* __wrap_readdir(DIR*);
int __wrap_closedir(DIR*);
int __wrap_dirfd(DIR*);
static int calls, closed, failure, malformed;
int sceKernelOpen(const char* path, int flags, int mode) {
    assert(!strcmp(path, "/data/YataiDON/Songs"));
    assert(flags == O_RDONLY && mode == 0);
    return failure ? (int)0x8002000d : 7;
}
int sceKernelClose(int fd) { assert(fd == 7); ++closed; return 0; }
int sceKernelGetdents(int fd, char* buffer, int size) {
    assert(fd == 7 && size == 65536);
    assert((uintptr_t)buffer % 4096 == 0);
    if (failure) return (int)0x80020005;
    if (calls++) return 0;
    struct dirent entries[2] = {0};
    entries[0].d_ino = 1;
    entries[0].d_type = DT_DIR;
    entries[0].d_reclen = malformed ? 0 : sizeof(struct dirent);
    strcpy(entries[0].d_name, "Charts");
    entries[1].d_ino = 2;
    entries[1].d_type = DT_REG;
    entries[1].d_reclen = sizeof(struct dirent);
    strcpy(entries[1].d_name, "test.tja");
    memcpy(buffer, entries, sizeof(entries));
    return sizeof(entries);
}
int main(void) {
    DIR* stream = ps5_native_opendir("/data/YataiDON/Songs");
    assert(stream && __wrap_dirfd(stream) == 7);
    struct dirent* entry = __wrap_readdir(stream);
    assert(entry && !strcmp(entry->d_name, "Charts") && entry->d_type == DT_DIR);
    entry = __wrap_readdir(stream);
    assert(entry && !strcmp(entry->d_name, "test.tja") && entry->d_type == DT_REG);
    errno = 0;
    assert(!__wrap_readdir(stream) && errno == 0);
    assert(__wrap_closedir(stream) == 0 && closed == 1);
    failure = 1;
    assert(!ps5_native_opendir("/data/YataiDON/Songs") && errno == EACCES);
    failure = 0; calls = 0; malformed = 1;
    stream = ps5_native_opendir("/data/YataiDON/Songs");
    assert(stream && !__wrap_readdir(stream) && errno == EIO);
    assert(__wrap_closedir(stream) == 0);
    malformed = 0; failure = 0;
    stream = ps5_native_opendir("/data/YataiDON/Songs");
    failure = 1;
    assert(!__wrap_readdir(stream) && errno == EIO);
    assert(__wrap_closedir(stream) == 0 && closed == 3);
    return 0;
}
