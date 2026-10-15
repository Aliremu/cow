#include <array>
#include <cstring>
#include <netinet/in.h>
#include <print>
#include <string>
#include <sys/socket.h>
#include <unistd.h>

auto main() -> int
{
    auto sock = socket(AF_INET, SOCK_STREAM, 0);
    auto addr = sockaddr_in{
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr =
            {
                .s_addr = INADDR_ANY,
            },
    };

    [[maybe_unused]] auto _ = connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

    auto message = "Hello, World!";
    send(sock, message, strlen(message), 0);

    close(sock);
}
