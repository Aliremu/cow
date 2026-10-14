#include <iostream>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>

// Lesson: shared_mutex admits many readers or one writer, which fits read-heavy
// data structures better than a plain mutex.
// Direction: read with shared_lock and update with unique_lock.
// Check: print 10 before the update and 15 after it.
// Pitfall: unordered_map access is not safe during a write, even for readers.
auto main() -> int
{
    std::unordered_map<std::string, int> scores = {{"Ada", 10}};
    std::shared_mutex scores_mutex;

    // Goal: allow readers to share the lock, while a writer has exclusive access.
    // TODO: Read Ada's score in a jthread under std::shared_lock.
    // TODO: Join the reader before changing the map.
    // TODO: Update Ada's score under std::unique_lock.
    // TODO: Read and print the updated score under std::shared_lock.
}
