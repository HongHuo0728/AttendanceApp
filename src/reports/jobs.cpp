#include "reports/jobs.h"
#include "core/constants.h"
#ifdef _MSC_VER
#include <concurrencysal.h>
#endif

namespace attendance {
namespace {

struct ReportJob {
    std::function<bool()> work;
    std::function<void(bool)> completion;
};
struct ReportResult {
    std::function<void(bool)> completion;
    bool success = false;
};

class ReportQueue {
public:
    ReportQueue() { pending.reserve(2); }
    ~ReportQueue() { Stop(); }

    bool HasCapacity() {
        std::lock_guard<std::mutex> lock(mutex);
        return !stopping && outstanding < 2;
    }

    bool Enqueue(HWND window, ReportJob job) {
        if (!window || !IsWindow(window) || !job.work || !job.completion) return false;
        std::lock_guard<std::mutex> lock(mutex);
        if (stopping || outstanding >= 2) return false;
        try {
            if (!worker.joinable()) worker = std::thread([this] { Run(); });
            owner = window;
            pending.push_back(std::move(job));
            ++outstanding;
            wake.notify_one();
            return true;
        } catch (const std::exception&) { return false; }
    }

    void Dispatch() {
        if (!ready.load(std::memory_order_acquire)) return;
        std::array<ReportResult, 2> results;
        size_t count = 0;
        {
            std::lock_guard<std::mutex> lock(mutex);
            count = completedCount;
            for (size_t i = 0; i < count; ++i) results[i] = std::move(completed[i]);
            completedCount = 0;
            outstanding -= count;
            ready.store(false, std::memory_order_release);
        }
        for (size_t i = 0; i < count; ++i) {
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (stopping || !IsWindow(owner)) break;
            }
            results[i].completion(results[i].success);
        }
    }

    void Stop() {
        {
            std::lock_guard<std::mutex> lock(mutex);
            stopping = true;
            owner = nullptr;
        }
        wake.notify_one();
        if (worker.joinable()) worker.join();
        std::lock_guard<std::mutex> lock(mutex);
        pending.clear();
        for (auto& result : completed) result = {};
        completedCount = 0;
        outstanding = 0;
        ready.store(false, std::memory_order_release);
    }

private:
    void Run() {
        for (;;) {
            ReportJob job;
            bool haveJob = false;
            {
                std::unique_lock<std::mutex> lock(mutex);
                wake.wait(lock, [this] { return stopping || !pending.empty(); });
                if (!pending.empty()) {
                    job = std::move(pending.front());
                    pending.erase(pending.begin());
                    haveJob = true;
                }
            }
#ifdef _MSC_VER
            // The analyzer loses unique_lock ownership across condition_variable::wait.
            _Analysis_assume_lock_not_held_(mutex);
#endif
            if (!haveJob) break;
            bool success = false;
            try { success = job.work(); } catch (const std::exception&) {}
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (!stopping && completedCount < completed.size()) {
                    completed[completedCount++] = {std::move(job.completion), success};
                    ready.store(true, std::memory_order_release);
                    // The main loop also drains results if the posted-message queue is full.
                    PostMessageW(owner, WM_APP_REPORT_READY, 0, 0);
                }
            }
        }
    }

    std::mutex mutex;
    std::condition_variable wake;
    std::vector<ReportJob> pending;
    std::array<ReportResult, 2> completed;
    size_t completedCount = 0;
    std::thread worker;
    std::atomic_bool ready{false};
    HWND owner = nullptr;
    size_t outstanding = 0;
    bool stopping = false;
};

ReportQueue& Reports() { static ReportQueue queue; return queue; }

} // namespace

bool CanQueueReport() { return Reports().HasCapacity(); }
bool QueueReport(HWND owner, std::function<bool()> work, std::function<void(bool)> completion) {
    return Reports().Enqueue(owner, {std::move(work), std::move(completion)});
}
bool QueuePreparedReport(HWND owner, const std::function<std::function<bool()>()>& prepare,
    std::function<void(bool)> completion) {
    if (!CanQueueReport()) return false;
    try { return QueueReport(owner, prepare(), std::move(completion)); }
    catch (const std::exception&) { return false; }
}
void DispatchReportCompletions() { Reports().Dispatch(); }
void StopReportJobs() { Reports().Stop(); }

} // namespace attendance
