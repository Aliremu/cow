#include "tcp.h"
#include <ranges>
#include <spdlog/spdlog.h>
#include <vector>

auto main() -> int
{
    auto addr = "0.0.0.0:8080";

    auto server = wow::TcpListener::bind(addr);
    if (!server)
    {
        spdlog::info("Failed to bind to server!");
        return 1;
    }
    spdlog::info("Listening on {}", addr);

    auto workers = std::vector<std::jthread>{};

    for (auto client : server->incoming())
    {
        workers.emplace_back(
            [client = std::move(client)]
            {
                auto buf = std::array<char, 1024>{};

                while (true)
                {
                    auto read = client->read(buf);
                    auto _ = client->write(buf | std::views::take(*read));
                }
            });
    }
}
