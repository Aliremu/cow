#include "tcp.h"
#include <spdlog/spdlog.h>

auto main() -> int
{
    auto client = wow::TcpStream::connect("127.0.0.1:8080");
    if (!client)
    {
        spdlog::info("Failed to connect to server!");
        return 1;
    }

    auto _ = client->write(std::string{"GET / HTTP/1.1"});
    auto buf = std::array<char, 1024>{};
    auto _ = client->read(buf);
    spdlog::info("Read: {}", buf);
}
