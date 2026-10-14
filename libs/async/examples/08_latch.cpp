#include <atomic>
#include <iostream>
#include <latch>
#include <thread>
#include <vector>

// Lesson: a latch is a one-shot countdown gate for a fixed number of events.
// Direction: each worker does its work then count_down(); main waits once.
// Check: completed is worker_count after all_finished.wait() returns.
// Pitfall: a latch cannot be reset; use a barrier for repeated phases.
auto main() -> int
{
    constexpr auto worker_count = 4;
    std::latch all_finished(worker_count);
    std::atomic<int> completed = 0;
    std::vector<std::jthread> workers;

    // Goal: wait until every worker reaches the same one-time checkpoint.
    // TODO: Start worker_count jthreads.
    // TODO: In each worker, increment completed and call all_finished.count_down().
    // TODO: In main, call all_finished.wait() before printing completed.
    //
    // Question: why is std::latch unsuitable for a second round?
}
