#pragma once

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
    [[nodiscard]] static auto from_listener(int socket, SocketAddr addr) -> TcpStream;
    [[nodiscard]] static auto connect(std::string_view addr) -> std::expected<TcpStream, std::string>;

    TcpStream(const TcpStream& other);
    TcpStream& operator=(const TcpStream& other);
    TcpStream(TcpStream&& other) noexcept;
    TcpStream& operator=(TcpStream&& other) noexcept;
    ~TcpStream();

    [[nodiscard]] auto peer_addr() const -> const SocketAddr&;

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
    [[nodiscard]] auto write(const T& buffer) const -> std::expected<std::size_t, std::string>
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

    TcpStream(int socket, SocketAddr addr, std::atomic<std::size_t>* count);
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

            iterator(TcpListener& listener);

            auto operator*() -> value_type;
            auto operator->() -> value_type;
            auto operator++() noexcept -> iterator&;
            auto operator++(int) noexcept -> iterator;

            [[nodiscard]] auto operator==(const iterator& other) const noexcept -> bool;

          private:
            TcpListener& m_Listener;
            mutable std::optional<std::expected<TcpStream, std::string>> m_Stream{std::nullopt};
        };

        IncomingStream(TcpListener& listener);

        [[nodiscard]] auto begin() -> iterator;
        [[nodiscard]] auto end() -> iterator;

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
