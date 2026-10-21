#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <limits.h>
#include <unistd.h>

long __wrap_pathconf(const char*, int);
char* __wrap_realpath(const char*, char*);

int __wrap_stat(const char*, struct stat*);
int __wrap_lstat(const char*, struct stat*);
int __wrap_fstat(int, struct stat*);
FILE* __wrap_fopen(const char*, const char*);
int __wrap_open(const char*, int, ...);
DIR* __wrap_opendir(const char*);
int __wrap_rename(const char*, const char*);
char* __wrap_getcwd(char*, size_t);
int __wrap_chdir(const char*);
static int result;
static const char* expected;
FILE* __real_fopen(const char* path, const char* mode) {
    assert(strcmp(path, expected) == 0 && strcmp(mode, "rb") == 0);
    return (FILE*)(uintptr_t)1;
}
FILE* __real_freopen(const char* path, const char* mode, FILE* stream) {
    (void)path; (void)mode; return stream;
}
int __real_open(const char* path, int flags, ...) {
    assert(strcmp(path, expected) == 0);
    if (flags & O_CREAT) {
        va_list args; va_start(args, flags);
        assert(va_arg(args, int) == 0644);
        va_end(args);
    }
    return 7;
}
DIR* ps5_native_opendir(const char* path) {
    assert(strcmp(path, expected) == 0);
    return (DIR*)(uintptr_t)1;
}
int __real_mkdir(const char* path, mode_t mode) { (void)path; (void)mode; return 0; }
int __real_unlink(const char* path) { (void)path; return 0; }
int __real_rmdir(const char* path) { (void)path; return 0; }
int __real_remove(const char* path) { (void)path; return 0; }
int __real_access(const char* path, int mode) { (void)path; (void)mode; return 0; }
char* __real_realpath(const char* path, char* output) { strcpy(output, path); return output; }
int __real_rename(const char* from, const char* to) {
    assert(strcmp(from, "/data/YataiDON/old") == 0);
    assert(strcmp(to, "/data/YataiDON/new") == 0);
    return 0;
}
int sceKernelStat(const char* path, struct stat* info) {
    assert(strcmp(path, expected) == 0);
    if (!result) info->st_size = 123;
    return result;
}
int sceKernelFstat(int fd, struct stat* info) {
    assert(fd == 7);
    if (!result) info->st_size = 456;
    return result;
}
int main(void) {
    struct stat info;
    expected = "/data/YataiDON/config.toml";
    result = (int32_t)0x80020002u;
    errno = EINVAL; // A stale unrelated errno must not turn ENOENT into a crash.
    assert(__wrap_stat("config.toml", &info) == -1 && errno == ENOENT);
    result = 0;
    assert(__wrap_stat("config.toml", &info) == 0 && info.st_size == 123);
    expected = "/app0/test";
    assert(__wrap_lstat("/app0/test", &info) == 0);
    assert(__wrap_fstat(7, &info) == 0 && info.st_size == 456);
    result = (int32_t)0x80020009u;
    assert(__wrap_fstat(7, &info) == -1 && errno == EBADF);
    assert(__wrap_stat(NULL, &info) == -1 && errno == EINVAL);
    assert(__wrap_stat("", &info) == -1 && errno == ENOENT);
    expected = "/data/YataiDON/config.toml";
    assert(__wrap_fopen("config.toml", "rb"));
    assert(__wrap_open("config.toml", O_RDONLY) == 7);
    assert(__wrap_open("config.toml", O_CREAT | O_WRONLY, 0644) == 7);
    expected = "/data/YataiDON/Songs";
    assert(__wrap_opendir("Songs"));
    assert(__wrap_rename("old", "new") == 0);
    assert(__wrap_chdir("/data/YataiDON") == 0);
    assert(__wrap_chdir("/app0") == -1 && errno == ENOTSUP);
    char cwd[128];
    assert(__wrap_getcwd(cwd, sizeof(cwd)) == cwd && !strcmp(cwd, "/data/YataiDON"));
    assert(!__wrap_getcwd(cwd, 2) && errno == ERANGE);
    char* allocated = __wrap_getcwd(NULL, 0);
    assert(allocated && !strcmp(allocated, "/data/YataiDON"));
    free(allocated);
    result = 0;
    expected = "/data/YataiDON/.";
    assert(__wrap_pathconf(".", _PC_PATH_MAX) == PATH_MAX);
    assert(__wrap_pathconf(".", _PC_NAME_MAX) == NAME_MAX);
    assert(__wrap_pathconf(".", -1) == -1 && errno == EINVAL);
    result = (int32_t)0x80020002u;
    assert(__wrap_pathconf(".", _PC_PATH_MAX) == -1 && errno == ENOENT);
    result = 0;
    expected = "/data/YataiDON/Songs";
    assert(__wrap_realpath("Songs/../Songs/.", cwd) == cwd && !strcmp(cwd, expected));
    char* canonical = __wrap_realpath("/data//YataiDON/./Songs", NULL);
    assert(canonical && !strcmp(canonical, expected));
    free(canonical);
    result = (int32_t)0x80020002u;
    assert(!__wrap_realpath("Songs", cwd) && errno == ENOENT);
}
