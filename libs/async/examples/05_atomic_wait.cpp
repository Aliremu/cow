#include <atomic>
#include <iostream>
#include <print>
#include <thread>

// Lesson: atomic::wait() blocks efficiently until an atomic value changes,
// without needing a mutex for this simple one-way handoff.
// Direction: wait for ready to differ from false, then notify after storing
// true. Start with the default order, then try release store/acquire load.
// Check: result should be 42 after the worker joins.
// Pitfall: notify_one() alone is insufficient; change the observed value too.
auto main() -> int
{
    std::atomic<bool> ready = false;
    std::atomic<int> result = 0;

    // Goal: wake one worker without using a mutex or condition_variable.
    // TODO: Create a jthread that calls ready.wait(false).
    auto thread = std::jthread{[&]
                               {
                                   ready.wait(false);
                                   result = 42;
                               }};
    // TODO: Have the worker store 42 in result after it wakes.
    // TODO: Store true in ready, then call ready.notify_one().
    ready.store(true);
    ready.notify_one();
    // TODO: Join the worker and print result.
    thread.join();
    //
    // Stretch: add explicit acquire/release memory orders and explain why.
    std::println("Reary: {}", result.load());
}
