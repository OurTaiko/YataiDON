#include "../src/libs/latest_worker.h"
#include <atomic>
#include <cassert>
#include <future>

int main() {
    std::promise<void> started, release, finished;
    auto gate = release.get_future().share();
    std::atomic<int> completed{0};
    {
        LatestWorker worker;
        worker.submit([&] { started.set_value(); gate.wait(); ++completed; });
        started.get_future().wait();
        // Browsing away replaces queued work without waiting for the active decoder.
        worker.submit([] { assert(false && "obsolete pending decode ran"); });
        worker.submit([&] { ++completed; finished.set_value(); });
        assert(completed == 0);
        release.set_value();
        finished.get_future().wait();
    }
    assert(completed == 2);
}
