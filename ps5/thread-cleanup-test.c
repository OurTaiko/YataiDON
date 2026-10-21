#include <assert.h>
#include <pthread.h>
#include <stdlib.h>

int __cxa_thread_atexit_impl(void (*)(void*), void*, void*);
static int first = 1, second = 2, reentrant = 3, main_value = 4;
static int sequence[4], count;

static void destroy(void* object) {
    int value = *(int*)object;
    sequence[count++] = value;
    if (value == 2) assert(__cxa_thread_atexit_impl(destroy, &reentrant, NULL) == 0);
}
static void* worker(void* unused) {
    (void)unused;
    assert(__cxa_thread_atexit_impl(destroy, &first, NULL) == 0);
    assert(__cxa_thread_atexit_impl(destroy, &second, NULL) == 0);
    return NULL;
}
static void check_main_cleanup(void) {
    assert(count == 4 && sequence[3] == 4);
}
int main(void) {
    assert(atexit(check_main_cleanup) == 0);
    pthread_t thread;
    assert(pthread_create(&thread, NULL, worker, NULL) == 0);
    assert(pthread_join(thread, NULL) == 0);
    assert(count == 3 && sequence[0] == 2 && sequence[1] == 3 && sequence[2] == 1);
    assert(__cxa_thread_atexit_impl(destroy, &main_value, NULL) == 0);
    return 0;
}
