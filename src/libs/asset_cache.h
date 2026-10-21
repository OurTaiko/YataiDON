#pragma once
#include <list>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>

// Thread-safe, byte-bounded retention. Eviction releases only the cache's
// reference; an asset currently used by a scene stays alive.
template<class T> class AssetCache {
    struct Entry {
        std::shared_ptr<T> value;
        size_t bytes;
        std::list<std::string>::iterator age;
    };
    std::mutex mutex;
    std::list<std::string> order;
    std::unordered_map<std::string, Entry> entries;
    size_t used = 0;
    const size_t budget;
public:
    explicit AssetCache(size_t budget) : budget(budget) {}
    std::shared_ptr<T> get(const std::string& key) {
        std::lock_guard lock(mutex);
        auto it = entries.find(key);
        if (it == entries.end()) return {};
        order.splice(order.begin(), order, it->second.age);
        return it->second.value;
    }
    void put(const std::string& key, std::shared_ptr<T> value, size_t bytes) {
        std::lock_guard lock(mutex);
        auto old = entries.find(key);
        if (old != entries.end()) {
            used -= old->second.bytes;
            order.erase(old->second.age);
            entries.erase(old);
        }
        if (!value || bytes > budget) return;
        while (used > budget - bytes && !order.empty()) {
            auto it = entries.find(order.back());
            used -= it->second.bytes;
            entries.erase(it);
            order.pop_back();
        }
        order.push_front(key);
        entries.emplace(key, Entry{std::move(value), bytes, order.begin()});
        used += bytes;
    }
    void clear() {
        std::lock_guard lock(mutex);
        entries.clear();
        order.clear();
        used = 0;
    }
};
