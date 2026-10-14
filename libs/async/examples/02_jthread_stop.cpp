#include <atomic>
#include <chrono>
#include <iostream>
#include <print>
#include <stop_token>
#include <thread>

using namespace std::chrono_literals;

// Lesson: std::jthread joins automatically and passes a stop_token to a
// callable that accepts one. Stopping is cooperative: the worker must check.
// Direction: loop while !token.stop_requested(), then call request_stop().
// Check: ticks must be greater than zero and stop increasing after the scope.
// Pitfall: a tight loop is a busy wait; add a small sleep when observing it.
auto main() -> int
{
    std::atomic<int> ticks = 0;

    // Goal: count until a stop request, then rely on jthread's RAII join.
    {
        // TODO: Create a std::jthread whose callable accepts std::stop_token.
        auto thread = std::jthread{[&ticks](std::stop_token token)
                                   {
                                       while (!token.stop_requested())
                                       {
                                           ticks.fetch_add(1);
                                       }
                                   }};
        // TODO: Increment ticks until stop.stop_requested() is true.

        // TODO: Sleep briefly, request_stop(), and leave the scope to join.
        std::this_thread::sleep_for(500ms);
        thread.request_stop();
    }
    // TODO: Print ticks after the jthread is destroyed.
    std::println("Ticks: {}", ticks.load(std::memory_order::relaxed));
}
