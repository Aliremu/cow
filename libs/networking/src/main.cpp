#include <expected>
#include <functional>
#include <spdlog/spdlog.h>
#include <sys/socket.h>
#include <type_traits>
#include <variant>

#include "tcp.h"

template <class... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};

namespace wow
{
template <typename... Ts>
auto visit(std::function_ref<std::variant<Ts...>> fn, const std::variant<Ts...>& value) -> void
{
    std::visit(fn, value);
}

template <typename Fn, typename Result, typename Err>
    requires requires(Fn&& fn, const Result& result, const Err& err) {
        fn(result);
        fn(err);
    }
auto visit(Fn&& fn, const std::expected<Result, Err>& value) -> void
{
    if (value)
    {
        std::forward<Fn>(fn)(value.value());
    }
    else
    {
        std::forward<Fn>(fn)(value.error());
    }
}
}

auto handle_contract_violation(const std::contracts::contract_violation& violation) -> void
{
    spdlog::info("Hello World");
}

auto main() -> int
{
    auto server = wow::TcpListener::bind("0.0.0.0:8080");
    if (!server)
    {
        spdlog::error("Error binding to address: {}", server.error());
        return -1;
    }

    spdlog::info("Listening on 0.0.0.0:8080");
    for (const auto& client : server->incoming())
    {
        wow::visit(
            overload{
                [](const wow::TcpStream& client)
                {
                    spdlog::info("Received connection from {}", client.Addr);
                    std::array<char, 1024> buffer{};
                    const auto received = ::recv(client.Socket, buffer.data(), buffer.size(), 0);
                    if (received == -1)
                    {
                        ::close(client.Socket);
                    }

                    spdlog::info("Received {} bytes", received);
                    constexpr auto response = "HTTP/1.1 200 OK\r\n"
                                              "Content-Type: text/plain; charset=utf-8\r\n"
                                              "Content-Length: 22\r\n"
                                              "Connection: close\r\n"
                                              "\r\n"
                                              "Hello from networking\n";
                    if (::send(client.Socket, response, std::strlen(response), 0) == -1)
                    {
                        ::close(client.Socket);
                    }
                },
                [](const std::string& error) { spdlog::error("Client error: ", error); }},
            client);
    }
    // server.route("/", Index{});
}
