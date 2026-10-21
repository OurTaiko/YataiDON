// Thread-local destructors for the statically linked title. No DSO unloading.
#include <pthread.h>
#include <stdlib.h>

typedef struct ThreadDestructor {
    void (*destroy)(void*);
    void* object;
    struct ThreadDestructor* next;
} ThreadDestructor;

static pthread_key_t destructor_key;
static pthread_once_t destructor_once = PTHREAD_ONCE_INIT;
static int key_error;

static void drain_destructors(void* initial) {
    // POSIX clears the key before calling us. Restore it so a destructor that
    // registers another destructor is picked up in the same LIFO drain.
    pthread_setspecific(destructor_key, initial);
    ThreadDestructor* entry;
    while ((entry = pthread_getspecific(destructor_key)) != NULL) {
        pthread_setspecific(destructor_key, entry->next);
        entry->destroy(entry->object);
        free(entry);
    }
}

static void drain_main_thread(void) {
    drain_destructors(pthread_getspecific(destructor_key));
}

static void make_destructor_key(void) {
    key_error = pthread_key_create(&destructor_key, drain_destructors);
    if (!key_error) atexit(drain_main_thread);
}

int __cxa_thread_atexit_impl(void (*destroy)(void*), void* object, void* dso) {
    (void)dso;
    if (pthread_once(&destructor_once, make_destructor_key) || key_error) return -1;
    ThreadDestructor* entry = malloc(sizeof(*entry));
    if (!entry) return -1;
    entry->destroy = destroy;
    entry->object = object;
    entry->next = pthread_getspecific(destructor_key);
    if (pthread_setspecific(destructor_key, entry)) {
        free(entry);
        return -1;
    }
    return 0;
}
