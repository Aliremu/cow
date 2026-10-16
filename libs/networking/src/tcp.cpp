#include "tcp.h"

#include <ranges>
#include <unistd.h>

namespace wow
{
auto SocketAddr::from_address(std::string_view addr) -> std::optional<SocketAddr>
{
    const auto separator = addr.find(':');
    if (separator == std::string_view::npos)
    {
        return std::nullopt;
    }

    const auto host_text = addr.substr(0, separator);
    const auto port_text = addr.substr(separator + 1);

    const auto parse_ipv4 = [](auto text) -> std::optional<in_addr_t>
    {
        auto ip = in_addr_t{};
        for (auto octet : text | std::views::split('.'))
        {
            auto value = 0;
            const auto [end, error] = std::from_chars(octet.data(), octet.data() + octet.size(), value);

            if (value < 0 || value > 255)
            {
                return std::nullopt;
            }

            ip = (ip << 8) | value;
        }

        return ip;
    };

    const auto parse_port = [](auto text) -> std::optional<in_addr_t>
    {
        auto port = uint16_t{};
        const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), port);

        if (error != std::errc{} || end != text.data() + text.size())
        {
            return std::nullopt;
        }

        return port;
    };

    const auto host = parse_ipv4(host_text);
    if (!host)
    {
        return std::nullopt;
    }

    const auto port = parse_port(port_text);
    if (!port)
    {
        return std::nullopt;
    }

    return std::optional<SocketAddr>{std::in_place, *host, *port};
}

auto TcpListener::incoming() -> IncomingStream
{
    return IncomingStream{*this};
}

auto TcpListener::bind(std::string_view addr) -> std::expected<TcpListener, std::string>
{
    const auto hostPort = SocketAddr::from_address(addr);
    if (!hostPort)
        return std::unexpected{"Failed to parse address!"};

    const auto server = ::socket(AF_INET, SOCK_STREAM, 0);
    if (server == -1)
        return std::unexpected{"Failed to create socket!"};

    const auto reuse_address = 1;
    if (::setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &reuse_address, sizeof(reuse_address)) == -1)
    {
        ::close(server);
        return std::unexpected{"Failed to setsockopt!"};
    }

    const auto address = sockaddr_in{
        .sin_family = AF_INET,
        .sin_port = ::htons(hostPort->Port),
        .sin_addr = {.s_addr = ::htonl(hostPort->Host)},
    };
    if (::bind(server, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == -1)
    {
        ::close(server);
        return std::unexpected{"Failed to bind server to address!"};
    }

    if (::listen(server, SOMAXCONN) == -1)
    {
        ::close(server);
        return std::unexpected{"Failed to begin listening!"};
    }

    return std::expected<TcpListener, std::string>{std::in_place, hostPort->Host, hostPort->Port, server};
}

TcpListener::TcpListener(in_addr_t host, uint16_t port, int socket)
    : m_Host{host}
    , m_Port{port}
    , m_Socket{socket}
{
}

TcpListener::~TcpListener()
{
    ::close(m_Socket);
}
}
