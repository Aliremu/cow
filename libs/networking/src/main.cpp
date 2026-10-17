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

template <typename T>
std::string to_json(T const& obj)
{
    std::string out = "{";
    bool first = true;
    constexpr auto ctx = std::meta::access_context::unchecked();
    template for (constexpr auto member : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx)))
    {
        if (!first)
            out += ',';
        first = false;
        out += std::format("{}:{}", std::meta::identifier_of(member), obj.[:member:]);
    }
    out += '}';
    return out;
}

struct index_controller
{
    auto get_name() -> std::string
    {
        return "Hello World!";
    }

    auto list_messages() -> wow::get_mapping<"/list">
    {
        auto name = get_name();
        return {200};
    }

    auto create_message(int id) -> wow::post_mapping<"/list">
    {
        return {200};
    }
};

struct message_controller
{
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
    spdlog::info("{}", to_json(Message{.Id = 1723, .Text = "Hello, World!"}));
    app.route<index_controller>("/").route<message_controller>("/messages");

    wow::serve(*server, app);
}
