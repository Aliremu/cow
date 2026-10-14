#include <atomic>
#include <iostream>
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
    // TODO: Have the worker store 42 in result after it wakes.
    // TODO: Store true in ready, then call ready.notify_one().
    // TODO: Join the worker and print result.
    //
    // Stretch: add explicit acquire/release memory orders and explain why.
}
