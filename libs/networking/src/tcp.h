#pragma once

#include <arpa/inet.h>
#include <contracts>
#include <cstdint>
#include <endian.h>
#include <expected>
#include <format>
#include <netinet/in.h>
#include <optional>
#include <string_view>
#include <sys/socket.h>

namespace wow
{

struct SocketAddr
{
    in_addr_t Host;
    uint16_t Port;

    [[nodiscard]] static auto from_address(std::string_view addr) -> std::optional<SocketAddr>;
};

struct TcpStream
{
    int Socket;
    SocketAddr Addr;
};

class TcpListener
{
  public:
    struct IncomingStream
    {
        struct iterator
        {
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::expected<TcpStream, std::string>;
            using difference_type = std::ptrdiff_t;
            using pointer = TcpStream*;
            using reference = TcpStream&;

            iterator(TcpListener& listener)
                : m_Listener{listener}
            {
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
                return *this;
            }

            auto operator++(int) noexcept -> iterator
            {
                iterator tmp = *this;
                ++(*this);
                return tmp;
            }

            [[nodiscard]] friend auto operator==(const iterator& a, const iterator& b) noexcept -> bool
            {
                auto addr = sockaddr_in{};
                auto len = socklen_t{sizeof(addr)};
                const auto client = ::accept(a.m_Listener.m_Socket, reinterpret_cast<sockaddr*>(&addr), &len);

                if (client == -1)
                {
                  a.m_Stream.emplace(std::unexpected{"Failed to accept client!"});
                }

                a.m_Stream.emplace(
                    std::in_place,
                    client,
                    SocketAddr{
                        .Host = ::ntohl(addr.sin_addr.s_addr),
                        .Port = ::ntohs(addr.sin_port),
                    });
                return false;
            }

          private:
            TcpListener& m_Listener;
            mutable std::optional<std::expected<TcpStream, std::string>> m_Stream{std::nullopt};
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

    [[nodiscard]] auto incoming() -> IncomingStream;
    [[nodiscard]] static auto bind(std::string_view addr) -> std::expected<TcpListener, std::string>;

    TcpListener(in_addr_t host, uint16_t port, int socket);
    ~TcpListener();

  private:
    friend class IncomingStream;
    friend class IncomingStream::iterator;

    in_addr_t m_Host;
    uint16_t m_Port;
    int m_Socket;
};
}

template <>
struct std::formatter<wow::SocketAddr> : std::formatter<std::string_view>
{
    auto format(const wow::SocketAddr& s, std::format_context& ctx) const
    {
        auto host = inet_ntoa(in_addr{s.Host});
        return std::formatter<std::string_view>::format(std::format("{}:{}", host, s.Port), ctx);
    }
};
