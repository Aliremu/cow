#include <arpa/inet.h>
#include <charconv>
#include <contracts>
#include <cstdint>
#include <endian.h>
#include <expected>
#include <netinet/in.h>
#include <optional>
#include <ranges>
#include <spdlog/spdlog.h>
#include <string_view>
#include <sys/socket.h>
#include <system_error>
#include <variant>

namespace
{

}

auto handle_contract_violation(const std::contracts::contract_violation& violation) -> void
{
    spdlog::info("Hello World");
}

class TcpListener
{
  public:
    struct SocketAddr
    {
        in_addr_t Host;
        uint16_t Port;

        [[nodiscard]] static auto from_address(std::string_view addr) -> std::optional<SocketAddr>
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
                    std::from_chars(octet.data(), octet.data() + octet.size(), value);
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
    };

    [[nodiscard]] static auto bind(std::string_view addr) -> std::expected<TcpListener, std::string>
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

    struct TcpStream
    {
        int Socket;
        SocketAddr Addr;
    };

    struct IncomingStream
    {
        struct iterator
        {
            using iterator_category = std::forward_iterator_tag;
            using value_type = TcpStream;
            using difference_type = std::ptrdiff_t;
            using pointer = TcpStream*;
            using reference = TcpStream&;

            iterator(TcpListener& listener)
                : m_Listener{listener}
            {
                auto addr = sockaddr_in{};
                const auto client = ::accept(m_Listener.m_Socket, reinterpret_cast<sockaddr*>(&addr), nullptr);

                m_Stream.emplace(
                    client,
                    SocketAddr{
                        .Host = addr.sin_addr.s_addr,
                        .Port = addr.sin_port,
                    });
            }

            auto operator*() -> value_type
            {
                auto ret = std::move(*m_Stream);
                m_Stream.reset();
                return ret;
            }
            auto operator->() -> value_type
            {
                auto ret = std::move(*m_Stream);
                m_Stream.reset();
                return ret;
            }

            auto operator++() noexcept -> iterator&
            {
                auto addr = sockaddr_in{};
                const auto client = ::accept(m_Listener.m_Socket, reinterpret_cast<sockaddr*>(&addr), nullptr);

                m_Stream.emplace(
                    client,
                    SocketAddr{
                        .Host = addr.sin_addr.s_addr,
                        .Port = addr.sin_port,
                    });
                return *this;
            }

            // 5. Postfix Increment (it++)
            auto operator++(int) noexcept -> iterator
            {
                iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            [[nodiscard]] friend auto operator==(const iterator& a, const iterator& b) noexcept -> bool
            {
                return false;
            }

          private:
            TcpListener& m_Listener;
            std::optional<TcpStream> m_Stream{std::nullopt};
        };

        IncomingStream(TcpListener& listener)
            : m_Listener{listener}
        {
        }

        [[nodiscard]] auto begin() -> iterator
        {
            return iterator{m_Listener};
        }

        [[nodiscard]] auto end() -> iterator
        {
            return iterator{m_Listener};
        }

      private:
        TcpListener& m_Listener;
    };

    [[nodiscard]] auto incoming() -> IncomingStream
    {
        return IncomingStream{*this};
    }

    TcpListener(in_addr_t host, uint16_t port, int socket)
        : m_Host{host}
        , m_Port{port}
        , m_Socket{socket}
    {
    }

  private:
    friend class IncomingStream;
    friend class IncomingStream::iterator;

    in_addr_t m_Host;
    uint16_t m_Port;
    int m_Socket;
};

struct Index
{
};

auto main() -> int
{
    auto server = TcpListener::bind("0.0.0.0:8080");
    if (!server)
    {
        spdlog::error(server.error());
        return -1;
    }

    spdlog::info("Listening on 0.0.0.0:8080");
    for (const auto& client : server->incoming())
    {
        spdlog::info("Received connection!");
    }
    // server.route("/", Index{});
}
