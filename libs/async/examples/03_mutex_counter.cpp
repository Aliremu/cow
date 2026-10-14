#include <iostream>
#include <mutex>
#include <print>
#include <ranges>
#include <thread>
#include <vector>

// Lesson: a mutex makes a critical section exclusive, preventing lost updates
// to ordinary shared data such as counter.
// Direction: use a lock_guard only around the increment, not the whole worker.
// Check: the final value must be worker_count * increments_per_worker.
// Pitfall: jthreads join when destroyed; print only after joining or leaving
// the scope that owns the vector.
auto main() -> int
{
    constexpr auto worker_count = 4;
    constexpr auto increments_per_worker = 10'000;

    auto counter = 0;
    {
        std::mutex counter_mutex;
        std::vector<std::jthread> workers;

        // Goal: end with worker_count * increments_per_worker.
        // TODO: Start worker_count jthreads.
        for (auto i : std::views::iota(0, worker_count))
        {
            workers.emplace_back(
                [&](std::stop_token token)
                {
                    for (auto i : std::views::iota(0, increments_per_worker))
                    {
                        auto lock = std::lock_guard{counter_mutex};
                        counter += 1;
                    }
                });
        }
    }
    // TODO: In each worker, increment counter increments_per_worker times.
    // TODO: Use std::lock_guard to protect every increment.
    // TODO: Print counter after the vector destroys and joins its jthreads.
    std::println("Counter: {}", counter);
}
