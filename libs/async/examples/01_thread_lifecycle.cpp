#include <print>
#include <thread>

// Lesson: a thread is an independently executing path; join() establishes a
// happens-before relationship, so reading result after join() is safe.
// Direction: implement the worker first, then join it, then print the value.
// Check: this should always print "Result: 42".
// Pitfall: destroying a joinable std::thread calls std::terminate().
auto main() -> int
{
    // Goal: start a worker that writes 42 to result, then print the result.
    auto result = 0;

    // TODO: Create a std::thread that writes 42 to result.
    auto thread = std::thread{[&result] { result = 42; }};
    // TODO: Join the thread before reading result.
    thread.join();
    // TODO: Print result.
    std::println("Result: {}", result);
}
