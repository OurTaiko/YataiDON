#pragma once
#include <atomic>
#include <type_traits>

namespace yd {
#if !defined(__cpp_lib_atomic_ref)
// Older standard libraries (e.g. the PS5 SDK's libc++) predate atomic_ref. Keep the mixer's existing
// atomic accesses using Clang's atomics, without adding declarations to std.
inline int atomic_order(std::memory_order order) {
    switch (order) {
    case std::memory_order_relaxed: return __ATOMIC_RELAXED;
    case std::memory_order_consume: return __ATOMIC_CONSUME;
    case std::memory_order_acquire: return __ATOMIC_ACQUIRE;
    case std::memory_order_release: return __ATOMIC_RELEASE;
    case std::memory_order_acq_rel: return __ATOMIC_ACQ_REL;
    default: return __ATOMIC_SEQ_CST;
    }
}
template<class T> class atomic_ref {
    static_assert(std::is_trivially_copyable_v<T>);
    static_assert(sizeof(T) == 1 || sizeof(T) == 4 || sizeof(T) == 8);
    T* value;
public:
    explicit atomic_ref(T& target) : value(&target) {}
    T load(std::memory_order order = std::memory_order_seq_cst) const {
        T result;
        __atomic_load(value, &result, atomic_order(order));
        return result;
    }
    void store(T next, std::memory_order order = std::memory_order_seq_cst) const {
        __atomic_store(value, &next, atomic_order(order));
    }
    T fetch_add(T amount, std::memory_order order = std::memory_order_seq_cst) const {
        return __atomic_fetch_add(value, amount, atomic_order(order));
    }
};
#else
template<class T> using atomic_ref = std::atomic_ref<T>;
#endif
}
