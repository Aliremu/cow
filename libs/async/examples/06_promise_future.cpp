#include <future>
#include <iostream>
#include <thread>

// Lesson: promise/future represents one result that will be supplied later.
// Direction: obtain the future before moving the promise into the worker.
// Check: future.get() blocks until it can print 42.
// Pitfall: a promise may be fulfilled exactly once; a second set_value throws.
auto main() -> int {
  std::promise<int> promise;
  auto future = promise.get_future();

  // Goal: deliver 42 from a worker to this thread.
  // TODO: Move promise into a std::jthread callable.
  // TODO: Call set_value(42) in the worker.
  // TODO: Call future.get() and print the returned value.
  //
  // Stretch: use set_exception() and handle the exception from get().
}
