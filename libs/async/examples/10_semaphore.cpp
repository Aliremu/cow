#include <atomic>
#include <chrono>
#include <iostream>
#include <print>
#include <ranges>
#include <semaphore>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// Lesson: a counting semaphore models a limited pool of interchangeable slots.
// Direction: pair each acquire() with exactly one release(), including on error.
// Check: four workers run, but peak_active never exceeds two.
// Pitfall: an unreleased slot can deadlock later workers; acquire before
// touching active and release after decrementing it.
auto main() -> int
{
    std::counting_semaphore<2> slots(2);
    std::atomic<int> active = 0;
    std::atomic<int> peak_active = 0;
    {
        std::vector<std::jthread> workers;

        // Goal: run four workers while allowing at most two in the critical section.
        // TODO: Start four jthreads.
        for (auto i : std::views::iota(0, 4))
        {
            workers.emplace_back(
                [&]
                {
                    slots.acquire();
                    auto count = active.fetch_add(1) + 1;
                    peak_active.fetch_max(count);
                    std::this_thread::sleep_for(200ms);
                    active.fetch_sub(1);
                    slots.release();
                });
        }
    }
    // TODO: acquire() a slot before incrementing active.
    // TODO: Record the largest observed active count in peak_active.
    // TODO: Sleep briefly, decrement active, and release() the slot.
    // TODO: Print peak_active after all workers join; it must be at most 2.
    std::println("Peak Active: {}", peak_active.load());
}
