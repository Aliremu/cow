#include <condition_variable>
#include <iostream>
#include <mutex>
#include <print>
#include <queue>
#include <thread>

// Lesson: a condition_variable lets a consumer sleep until shared state
// changes, instead of repeatedly polling a queue.
// Direction: guard jobs and producer_finished with mutex, then use wait() with
// a predicate. Remove a job while locked, but process it after unlocking.
// Check: producing 1, 2, and 3 should print a total of 6.
// Pitfall: notify_one() does not preserve a condition; the predicate does.
auto main() -> int
{
    std::mutex mutex;
    std::condition_variable ready;
    std::queue<int> jobs;
    auto producer_finished = false;
    auto total = 0;

    // Goal: have a producer send 1, 2, and 3; the consumer should print 6.
    // TODO: Create a consumer that waits with a predicate:
    //       !jobs.empty() || producer_finished.
    auto consumer = std::jthread{[&](std::stop_token token)
                                 {
                                     while (true)
                                     {
                                         auto lock = std::unique_lock{mutex};
                                         ready.wait(lock, [&] { return !jobs.empty() || producer_finished; });

                                         if (jobs.empty())
                                         {
                                             return;
                                         }

                                         auto job = jobs.front();
                                         jobs.pop();
                                         lock.unlock();
                                         total += job;
                                     }
                                 }};
    // TODO: Pop a job while holding the lock, then unlock before adding to total.
    // TODO: Exit only when the queue is empty and producer_finished is true.
    // TODO: Create a producer that pushes jobs under the mutex and notify_one()s.
    auto producer = std::jthread{[&](std::stop_token token)
                                 {
                                     auto lock = std::lock_guard{mutex};
                                     jobs.push(1);
                                     ready.notify_one();
                                     jobs.push(2);
                                     ready.notify_one();
                                     jobs.push(3);
                                     ready.notify_one();

                                     producer_finished = true;
                                     ready.notify_one();
                                 }};
    // TODO: Set producer_finished under the mutex and notify the consumer.
    producer.join();
    consumer.join();

    std::println("Total: {}", total);
}
