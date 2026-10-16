#include <chrono>
#include <future>
#include <print>
#include <thread>

using namespace std::chrono_literals;

// Lesson: std::async pairs task launch with a future for its eventual result.
// Direction: specify std::launch::async so this exercise starts concurrently.
// Check: get() waits for the task and produces 42.
// Pitfall: without an explicit launch policy, an implementation may defer work
// until get() rather than creating concurrent execution.
auto main() -> int
{
    // Goal: start a task immediately and collect its result.
    // TODO: Call std::async with std::launch::async.
    auto task = std::async(
        std::launch::async,
        []
        {
            std::this_thread::sleep_for(20ms);
            return 42;
        });
    // TODO: Make the callable sleep for 20ms and return 42.
    // TODO: Print task.get().
    std::println("Result: {}", task.get());
    //
    // Question: what can happen if std::launch::async is omitted?
}
