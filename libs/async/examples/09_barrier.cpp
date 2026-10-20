#include <algorithm>
#include <atomic>
#include <barrier>
#include <print>
#include <ranges>
#include <thread>
#include <vector>

// Lesson: a barrier synchronizes the same group repeatedly at phase boundaries.
// Direction: every participant must arrive at each barrier exactly once per
// phase. Let the vector join workers before inspecting the final count.
// Check: second_phase_completed should equal participant_count.
// Pitfall: a participant that skips a phase leaves every other participant
// blocked at that barrier.
auto main() -> int
{
    constexpr auto participant_count = 3;
    std::barrier phase(participant_count);
    std::atomic<int> second_phase_completed = 0;
    std::vector<std::jthread> participants;

    // Goal: synchronize the same workers at two phase boundaries.
    // TODO: Start participant_count jthreads.
    for (auto i : std::views::iota(0, participant_count))
    {
        participants.emplace_back(
            [&]
            {
                phase.arrive_and_wait();
                second_phase_completed.fetch_add(1);
                phase.arrive_and_wait();
            });
    }
    // TODO: In every thread, call phase.arrive_and_wait().
    // TODO: Increment second_phase_completed, then arrive_and_wait() again.
    // TODO: Print the count after the jthreads have joined.
    std::ranges::for_each(participants, [](std::jthread& j) { j.join(); });
    std::println("Second Phase: {}", second_phase_completed.load());

    // Question: how does this differ from std::latch?
}
