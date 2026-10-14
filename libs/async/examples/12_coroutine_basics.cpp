#include <coroutine>
#include <iostream>
#include <utility>

// Lesson: a coroutine stores suspended execution in a coroutine frame. Its
// promise holds the result, and its handle resumes or destroys the frame.
// Direction: get() should resume this lazy task once, then read value_.
// Check: answer().get() should print 42.
// Pitfall: every owning coroutine handle must be destroyed exactly once.
class task {
 public:
  struct promise_type {
    auto get_return_object() -> task {
      return task{std::coroutine_handle<promise_type>::from_promise(*this)};
    }

    auto initial_suspend() -> std::suspend_always { return {}; }
    auto final_suspend() noexcept -> std::suspend_always { return {}; }
    auto return_value(int value) -> void { value_ = value; }
    auto unhandled_exception() -> void { std::terminate(); }

    int value_ = 0;
  };

  explicit task(std::coroutine_handle<promise_type> handle) : handle_(handle) {}
  task(task&& other) noexcept : handle_(std::exchange(other.handle_, {})) {}
  task(const task&) = delete;
  auto operator=(const task&) -> task& = delete;

  ~task() {
    if (handle_) {
      handle_.destroy();
    }
  }

  auto get() -> int {
    // TODO: Resume the coroutine once and return promise().value_.
    return 0;
  }

 private:
  std::coroutine_handle<promise_type> handle_;
};

auto answer() -> task {
  // TODO: Return 42 with co_return.
  co_return 0;
}

auto main() -> int {
  auto result = answer();
  // TODO: Print result.get().
}
