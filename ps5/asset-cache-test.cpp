#include "../src/libs/asset_cache.h"
#include <atomic>
#include <cassert>
#include <thread>
#include <vector>

struct Resource {
    inline static std::atomic<int> live{0};
    Resource() { ++live; }
    ~Resource() { --live; }
};
int main() {
    AssetCache<Resource> cache(8);
    auto pinned = std::make_shared<Resource>();
    cache.put("playing", pinned, 4);
    cache.put("older", std::make_shared<Resource>(), 4);
    assert(cache.get("playing") == pinned);
    cache.put("next", std::make_shared<Resource>(), 4);
    assert(!cache.get("older"));
    cache.put("replacement", std::make_shared<Resource>(), 8);
    assert(!cache.get("playing") && Resource::live == 2);
    cache.clear();
    assert(Resource::live == 1); // eviction must not free a playing resource
    pinned.reset();
    assert(Resource::live == 0);
    cache.put("huge", std::make_shared<Resource>(), 9);
    assert(!cache.get("huge") && Resource::live == 0);
    for (int i = 0; i < 100; ++i) cache.put("same", std::make_shared<Resource>(), 1);
    assert(Resource::live == 1);
    std::vector<std::thread> workers;
    for (int i = 0; i < 4; ++i) workers.emplace_back([&cache, i] {
        for (int j = 0; j < 1000; ++j) {
            const auto key = std::to_string((i + j) % 12);
            cache.put(key, std::make_shared<Resource>(), 1);
            auto asset = cache.get(key);
        }
    });
    for (auto& worker : workers) worker.join();
    cache.clear();
    assert(Resource::live == 0);
}
