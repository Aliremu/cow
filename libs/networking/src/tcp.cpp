#include "tcp.h"

#include <array>
#include <charconv>
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

auto TcpStream::from_listener(int socket, SocketAddr addr) -> TcpStream
{
    return TcpStream{
        socket,
        addr,
        new std::atomic<std::size_t>(1),
    };
}

TcpStream::TcpStream(const TcpStream& other)
    : m_Socket{other.m_Socket}
    , m_Addr{other.m_Addr}
    , m_Count{other.m_Count}
{
    m_Count->fetch_add(1, std::memory_order::relaxed);
}

auto TcpStream::operator=(const TcpStream& other) -> TcpStream&
{
    m_Socket = other.m_Socket;
    m_Addr = other.m_Addr;
    m_Count = other.m_Count;
    m_Count->fetch_add(1, std::memory_order::relaxed);
    return *this;
}

TcpStream::TcpStream(TcpStream&& other) noexcept
    : m_Socket{std::exchange(other.m_Socket, -1)}
    , m_Addr{other.m_Addr}
    , m_Count{std::exchange(other.m_Count, nullptr)}
{
}

auto TcpStream::operator=(TcpStream&& other) noexcept -> TcpStream&
{
    m_Socket = std::exchange(other.m_Socket, -1);
    m_Addr = other.m_Addr;
    m_Count = std::exchange(other.m_Count, nullptr);
    return *this;
}

TcpStream::~TcpStream()
{
    if (m_Count && m_Count->fetch_sub(1, std::memory_order::acq_rel) == 1)
    {
        ::close(m_Socket);
        delete m_Count;
    }
}

auto TcpStream::peer_addr() const -> const SocketAddr&
{
    return m_Addr;
}

TcpStream::TcpStream(int socket, SocketAddr addr, std::atomic<std::size_t>* count)
    : m_Socket{socket}
    , m_Addr{addr}
    , m_Count{count}
{
}

TcpListener::IncomingStream::iterator::iterator(TcpListener& listener)
    : m_Listener{listener}
{
}

auto TcpListener::IncomingStream::iterator::operator*() -> value_type
{
    auto result = std::move(*m_Stream);
    m_Stream.reset();
    return result;
}

auto TcpListener::IncomingStream::iterator::operator->() -> value_type
{
    auto result = std::move(*m_Stream);
    m_Stream.reset();
    return result;
}

auto TcpListener::IncomingStream::iterator::operator++() noexcept -> iterator&
{
    return *this;
}

auto TcpListener::IncomingStream::iterator::operator++(int) noexcept -> iterator
{
    auto copy = *this;
    ++(*this);
    return copy;
}

auto TcpListener::IncomingStream::iterator::operator==(const iterator&) const noexcept -> bool
{
    auto address = sockaddr_in{};
    auto length = socklen_t{sizeof(address)};
    const auto client = ::accept(m_Listener.m_Socket, reinterpret_cast<sockaddr*>(&address), &length);

    if (client == -1)
    {
        m_Stream.emplace(std::unexpected{"Failed to accept client!"});
        return false;
    }

    m_Stream.emplace(
        std::in_place,
        TcpStream::from_listener(
            client,
            SocketAddr{
                .Host = ::ntohl(address.sin_addr.s_addr),
                .Port = ::ntohs(address.sin_port),
            }));
    return false;
}

TcpListener::IncomingStream::IncomingStream(TcpListener& listener)
    : m_Listener{listener}
{
}

auto TcpListener::IncomingStream::begin() -> iterator
{
    return iterator{m_Listener};
}

auto TcpListener::IncomingStream::end() -> iterator
{
    return iterator{m_Listener};
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

auto serve(TcpListener& listener, Router& router) -> void
{
    for (const auto& client : listener.incoming())
    {
        wow::visit(
            overload{
                [&router](const TcpStream& client)
                {
                    std::array<char, 1024> buffer{};
                    const auto read = client.read(buffer);

                    const auto request = Request::from_raw(std::string_view{buffer});
                    if (!request)
                    {
                        return;
                    }

                    const auto response = router.dispatch(*request);
                    const auto written = client.write(response.build_response());
                    static_cast<void>(read);
                    static_cast<void>(written);
                },
                [](const std::string&) {},
            },
            client);
    }
}
}
