# Async examples

Each source in `examples/` is a buildable exercise shell. Read its goal and
TODOs, implement them, then run it from the repository root with:

```sh
make async-example EXAMPLE=thread-lifecycle
```

| Example | Concept |
| --- | --- |
| `thread-lifecycle` | Create a `std::thread` and synchronize with `join()` |
| `jthread-stop` | RAII joining and cooperative cancellation with `std::jthread` |
| `mutex-counter` | Protect shared mutable state with `std::mutex` |
| `condition-variable` | Coordinate a producer and consumer without busy waiting |
| `atomic-wait` | Block on an atomic value with `wait()` and `notify_one()` |
| `promise-future` | Pass a one-shot result between threads |
| `async-task` | Launch and collect a task with `std::async` |
| `latch` | Wait for a fixed set of workers to finish |
| `barrier` | Synchronize workers at repeated phase boundaries |
| `semaphore` | Limit concurrent access to a fixed number of slots |
| `shared-mutex` | Allow concurrent readers while excluding writers |
| `coroutine-basics` | Build and consume a minimal lazy coroutine |

Read and implement them in order. The unmodified shells either do nothing or
return a placeholder value so you can compile one while working on another.
Change one thing at a time, rebuild, and observe the result.

Every exercise starts with four kinds of comments:

- **Lesson** explains the synchronization tool and when it is appropriate.
- **Direction** gives the intended implementation order without giving code.
- **Check** states an observable result that confirms the exercise works.
- **Pitfall** names the common bug the exercise is meant to prevent.
