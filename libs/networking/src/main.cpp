#include <array>
#include <atomic>
#include <expected>
#include <functional>
#include <memory>
#include <meta>
#include <spdlog/spdlog.h>
#include <string>
#include <sys/socket.h>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

#include "router.h"
#include "tcp.h"

auto handle_contract_violation(const std::contracts::contract_violation& violation) -> void
{
    spdlog::info("Hello World");
}

struct Message
{
    uint64_t Id;
    std::string Text;
};

struct AppState
{
    std::vector<Message> Messages;
};

namespace index_controller
{
    [[= wow::Mapping{""}]] auto index() -> wow::Response
    {
        return wow::ok(10);
    }
}

// struct message_controller
// {
//     auto get_name() -> std::string
//     {
//         return "Hello World!";
//     }
//
//     [[= wow::Mapping{""}]] auto list_messages() -> wow::Response
//     {
//         auto name = get_name();
//         return {200};
//     }
//
//     [[= wow::Mapping{""}]] auto create_message() -> wow::Response
//     {
//         return {200};
//     }
//
//     [[= wow::Mapping{"/{id}"}]] auto get_message([[= wow::Mapping{"id"}]] int id) -> wow::Response
//     {
//         return wow::ok(5);
//     }
// };

template <typename T>
struct AtomicMirror
{
    struct type;

    consteval
    {
        using namespace std::meta;
        std::vector<info> atomic_members;
        const auto context = access_context::unchecked();

        for (const info member : nonstatic_data_members_of(^^T, context))
        {
            const info original_type = type_of(member);
            info mirrored_type = original_type;

            // std::atomic<T> requires T to be trivially copyable. Preserve
            // fields such as std::string instead of forming an invalid type.
            if (is_trivially_copyable_type(original_type))
                mirrored_type = substitute(^^std::atomic, std::array{original_type});

            atomic_members.push_back(data_member_spec(
                mirrored_type,
                {.name = identifier_of(member)}));
        }

        define_aggregate(^^type, atomic_members);
    }
};

auto main() -> int
{
    auto server = wow::TcpListener::bind("0.0.0.0:8080");
    if (!server)
    {
        spdlog::error("Error binding to address: {}", server.error());
        return -1;
    }

    spdlog::info("Listening on 0.0.0.0:8080");
    auto app = wow::Router{};
    spdlog::info("{}", wow::to_json(Message{.Id = 1723, .Text = "Hello, World!"}));
    app.route<^^index_controller>("/"); //.route<message_controller>("/messages");

    using ConcurrentConfig = AtomicMirror<Message>::type;
    auto c = ConcurrentConfig{};
    c.Id.store(10);
    spdlog::info("Id: {}", c.Id.load());

    wow::serve(*server, app);
}
