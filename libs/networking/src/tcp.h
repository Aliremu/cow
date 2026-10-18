#pragma once

#include "http.h"
#include "router.h"
#include <arpa/inet.h>
#include <atomic>
#include <concepts>
#include <contracts>
#include <cstddef>
#include <cstdint>
#include <endian.h>
#include <expected>
#include <format>
#include <functional>
#include <netinet/in.h>
#include <optional>
#include <print>
#include <string_view>
#include <sys/socket.h>
#include <type_traits>
#include <unistd.h>
#include <utility>
#include <variant>

namespace wow
{

struct SocketAddr
{
    in_addr_t Host;
    uint16_t Port;

    [[nodiscard]] static auto from_address(std::string_view addr) -> std::optional<SocketAddr>;
};

class TcpStream
{
  public:
    [[nodiscard]] static auto from_listener(int socket, SocketAddr addr) -> TcpStream
    {
        return {
            socket,
            addr,
            new std::atomic<std::size_t>(1),
        };
    }

    TcpStream(const TcpStream& other)
        : m_Socket{other.m_Socket}
        , m_Addr{other.m_Addr}
        , m_Count{other.m_Count}
    {
        m_Count->fetch_add(1, std::memory_order::relaxed);
    }
    TcpStream& operator=(const TcpStream& other)
    {
        m_Socket = other.m_Socket;
        m_Addr = other.m_Addr;
        m_Count = other.m_Count;
        m_Count->fetch_add(1, std::memory_order::relaxed);

        return *this;
    }

    TcpStream(TcpStream&& other) noexcept
        : m_Socket{std::exchange(other.m_Socket, -1)}
        , m_Addr{other.m_Addr}
        , m_Count{std::exchange(other.m_Count, nullptr)}
    {
    }

    TcpStream& operator=(TcpStream&& other) noexcept
    {
        m_Socket = std::exchange(other.m_Socket, -1);
        m_Addr = other.m_Addr;
        m_Count = std::exchange(other.m_Count, nullptr);

        return *this;
    }

    ~TcpStream()
    {
        if (m_Count && m_Count->fetch_sub(1, std::memory_order::acq_rel) == 1)
        {
            ::close(m_Socket);
            delete m_Count;
        }
        m_Count = nullptr;
    }

    [[nodiscard]] auto peer_addr() const -> const SocketAddr&
    {
        return m_Addr;
    }

    template <typename T>
        requires requires(T& buffer) {
            { buffer.size() } -> std::integral;
            buffer.data();
            requires std::is_pointer_v<decltype(buffer.data())>;
        }
    [[nodiscard]] auto read(T& buffer) const -> std::expected<std::size_t, std::string>
    {
        const auto received = ::recv(m_Socket, static_cast<void*>(buffer.data()), buffer.size(), 0);
        if (received == -1)
        {
            ::close(m_Socket);
            return std::unexpected{"Failed to read from socket!"};
        }

        return received;
    }

    template <typename T>
        requires requires(T& buffer) {
            { buffer.size() } -> std::integral;
            buffer.data();
            requires std::is_pointer_v<decltype(buffer.data())>;
        }
    [[nodiscard]] auto write(T& buffer) const -> std::expected<std::size_t, std::string>
    {
        const auto written = ::send(m_Socket, static_cast<const void*>(buffer.data()), buffer.size(), 0);
        if (written == -1)
        {
            ::close(m_Socket);
            return std::unexpected{"Failed to write to socket!"};
        }

        return written;
    }

  private:
    int m_Socket;
    SocketAddr m_Addr;
    std::atomic<std::size_t>* m_Count;

    TcpStream(int socket, SocketAddr addr, std::atomic<std::size_t>* count)
        : m_Socket{socket}
        , m_Addr{addr}
        , m_Count{count}
    {
    }
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
                    return false;
                }

                a.m_Stream.emplace(
                    std::in_place,
                    TcpStream::from_listener(
                        client,
                        SocketAddr{
                            .Host = ::ntohl(addr.sin_addr.s_addr),
                            .Port = ::ntohs(addr.sin_port),
                        }));
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

template <class... Ts>
struct overload : Ts...
{
    using Ts::operator()...;
};

template <typename... Ts>
inline auto visit(std::function_ref<std::variant<Ts...>> fn, const std::variant<Ts...>& value) -> void
{
    std::visit(fn, value);
}

template <typename Fn, typename Result, typename Err>
    requires requires(Fn&& fn, const Result& result, const Err& err) {
        fn(result);
        fn(err);
    }
inline auto visit(Fn&& fn, const std::expected<Result, Err>& value) -> void
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

inline auto serve(TcpListener& listener, Router& router) -> void
{
    for (const auto& client : listener.incoming())
    {
        wow::visit(
            overload{
                [](const wow::TcpStream& client)
                {
                    // spdlog::info("Received connection from {}", client.peer_addr());
                    std::array<char, 1024> buffer{};
                    auto read = client.read(buffer);

                    auto request = Request::from_raw(std::string_view{buffer});
                    if (request)
                    {
                        std::println(
                            "{} {} {}\n{}\n\n{}",
                            request->Method,
                            request->Target,
                            request->Version,
                            request->Headers |
                                std::views::transform([](const auto& a) { return std::format("{}", a); }) |
                                std::views::join_with('\n') | std::ranges::to<std::string>(),
                            request->Body);
                    }
                    // wow::visit(
                    //     overload{
                    //         [&](std::size_t len) { std::println("Received {} bytes: {}", len, buffer); },
                    //         [](const std::string& error) { std::println("{}", error); },
                    //     },
                    //     read);

                    const auto response = std::string{"HTTP/1.1 200 OK\r\n"
                                                      "Content-Type: text/plain; charset=utf-8\r\n"
                                                      "Content-Length: 22\r\n"
                                                      "Connection: close\r\n"
                                                      "\r\n"
                                                      "Hello from networking\n"};
                    // spdlog::info("Response: {}", response);

                    auto _ = client.write(response);
                },
                [](const std::string& error)
                {
                    // spdlog::error("Client error: ", error);
                },
            },
            client);
    }
}
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
