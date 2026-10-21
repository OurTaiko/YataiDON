#define open test_open
#define fstat test_fstat
#include "native-file-data.c"
#undef open
#undef fstat
#include <assert.h>
#include <string.h>

static int open_error, stat_error, read_error, interrupt_once, closed, reads;
static size_t file_length, position, partial;
int test_open(const char* path, int flags, ...) {
    assert(!strcmp(path, "asset.png") && flags == O_RDONLY);
    position = 0;
    if (open_error) { errno = ENOENT; return -1; }
    return 7;
}
int test_fstat(int fd, struct stat* info) {
    assert(fd == 7);
    if (stat_error) { errno = EACCES; return -1; }
    memset(info, 0, sizeof(*info));
    info->st_mode = S_IFREG;
    info->st_size = file_length;
    return 0;
}
ssize_t sceKernelRead(int fd, void* output, size_t length) {
    assert(fd == 7 && length <= (1u << 20));
    ++reads;
    if (interrupt_once) { interrupt_once = 0; return (int32_t)0x80020004; }
    if (read_error) return (int32_t)0x80020005;
    if (partial && length > partial) length = partial;
    for (size_t i = 0; i < length; ++i) ((unsigned char*)output)[i] = (position + i) & 255;
    position += length;
    return length;
}
int sceKernelClose(int fd) { assert(fd == 7); ++closed; return 0; }
int main(void) {
    int size = 99;
    file_length = (2u << 20) + 17;
    unsigned char* data = ps5_load_file_data("asset.png", &size);
    assert(data && size == (int)file_length && reads == 3 && closed == 1);
    for (int i = 0; i < size; ++i) assert(data[i] == (i & 255));
    assert(data[size] == 0);
    free(data);
    file_length = 23; partial = 7; interrupt_once = 1;
    data = ps5_load_file_data("asset.png", &size);
    assert(data && size == 23 && data[22] == 22 && closed == 2);
    free(data);
    read_error = 1;
    assert(!ps5_load_file_data("asset.png", &size) && size == 0 && errno == EIO && closed == 3);
    read_error = 0; stat_error = 1;
    assert(!ps5_load_file_data("asset.png", &size) && errno == EACCES && closed == 4);
    stat_error = 0; open_error = 1;
    assert(!ps5_load_file_data("asset.png", &size) && errno == ENOENT && closed == 4);
    open_error = 0; file_length = 0;
    char* text = ps5_load_file_text("asset.png");
    assert(text && text[0] == 0 && closed == 5);
    free(text);
    file_length = (size_t)INT_MAX + 1;
    assert(!ps5_load_file_data("asset.png", &size) && errno == EFBIG && closed == 6);
}
