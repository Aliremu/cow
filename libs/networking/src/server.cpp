#include <array>
#include <cerrno>
#include <cstring>
#include <netinet/in.h>
#include <print>
#include <sys/socket.h>
#include <unistd.h>

auto main() -> int
{
    const auto server = socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1)
    {
        std::println("socket");
        return 1;
    }

    const auto reuse_address = 1;
    if (setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address)) == -1)
    {
        std::println("setsockopt");
        close(server);
        return 1;
    }

    const auto address = sockaddr_in{
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr = {.s_addr = htonl(INADDR_ANY)},
    };
    if (bind(server, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1)
    {
        std::println("bind");
        close(server);
        return 1;
    }
    if (listen(server, 5) == -1)
    {
        std::println("listen");
        close(server);
        return 1;
    }

    std::println("Listening on 0.0.0.0:8080");
    const auto client = accept(server, nullptr, nullptr);
    if (client == -1)
    {
        std::println("accept");
        close(server);
        return 1;
    }

    std::array<char, 1024> buffer{};
    const auto received = recv(client, buffer.data(), buffer.size(), 0);
    if (received == -1)
    {
        std::println("recv");
        close(client);
        close(server);
        return 1;
    }

    std::println("Received {} bytes", received);
    constexpr auto response = "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/plain; charset=utf-8\r\n"
                              "Content-Length: 22\r\n"
                              "Connection: close\r\n"
                              "\r\n"
                              "Hello from networking\n";
    if (send(client, response, std::strlen(response), 0) == -1)
    {
        std::println("send");
        close(client);
        close(server);
        return 1;
    }

    close(client);
    close(server);
}
