#include <atomic>
#include <barrier>
#include <iostream>
#include <thread>
#include <vector>

// Lesson: a barrier synchronizes the same group repeatedly at phase boundaries.
// Direction: every participant must arrive at each barrier exactly once per
// phase. Let the vector join workers before inspecting the final count.
// Check: second_phase_completed should equal participant_count.
// Pitfall: a participant that skips a phase leaves every other participant
// blocked at that barrier.
auto main() -> int {
  constexpr auto participant_count = 3;
  std::barrier phase(participant_count);
  std::atomic<int> second_phase_completed = 0;
  std::vector<std::jthread> participants;

  // Goal: synchronize the same workers at two phase boundaries.
  // TODO: Start participant_count jthreads.
  // TODO: In every thread, call phase.arrive_and_wait().
  // TODO: Increment second_phase_completed, then arrive_and_wait() again.
  // TODO: Print the count after the jthreads have joined.
  //
  // Question: how does this differ from std::latch?
}
