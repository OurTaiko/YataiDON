#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <sys/mman.h>

void* __wrap_mmap(void*, size_t, int, int, int, off_t);
int __wrap_munmap(void*, size_t);
int __wrap_mprotect(void*, size_t, int);
static int result;
static char allocation;
int sceKernelMmap(void* address, size_t length, int prot, int flags,
                  int fd, off_t offset, void** mapped) {
    assert(address == 0 && length == 0x40000);
    assert(prot == (PROT_READ | PROT_WRITE));
    assert(flags == (MAP_PRIVATE | MAP_ANONYMOUS));
    assert(fd == -1 && offset == 0);
    if (!result) *mapped = &allocation;
    return result;
}
int sceKernelMunmap(void* address, size_t length) {
    assert(address == &allocation && length == 0x40000);
    return result;
}
int sceKernelMprotect(const void* address, size_t length, int prot) {
    assert(address == &allocation && length == 0x40000 && prot == PROT_READ);
    return result;
}
int main(void) {
    assert(__wrap_mmap(0, 0x40000, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) == &allocation);
    assert(__wrap_munmap(&allocation, 0x40000) == 0);
    assert(__wrap_mprotect(&allocation, 0x40000, PROT_READ) == 0);
    result = (int)0x8002000c;
    assert(__wrap_mmap(0, 0x40000, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0) == MAP_FAILED);
    assert(errno == 12);
    result = (int)0x80020016;
    assert(__wrap_munmap(&allocation, 0x40000) == -1 && errno == 22);
    assert(__wrap_mprotect(&allocation, 0x40000, PROT_READ) == -1 && errno == 22);
    return 0;
}
