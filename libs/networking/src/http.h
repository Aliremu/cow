#pragma once

#include <concepts>
#include <format>
#include <meta>
#include <optional>
#include <print>
#include <ranges>
#include <string_view>
#include <tuple>
#include <vector>
#include <fstream>

#include "common.h"

namespace wow
{

using RequestLine = std::tuple<std::string_view, std::string_view, std::string_view>;

struct Header
{
    std::string_view Key;
    std::string_view Value;
};

namespace detail
{
inline auto parse_request_line(const auto& line) -> std::optional<RequestLine>
{
    auto parts = line | std::views::split(' ');
    auto it = parts.begin();
    if (it == parts.end())
        return std::nullopt;

    auto method = std::string_view{*it};

    ++it;
    if (it == parts.end())
        return std::nullopt;

    auto target = std::string_view{*it};

    ++it;
    if (it == parts.end())
        return std::nullopt;

    auto version = std::string_view{*it};

    return std::make_optional<RequestLine>(method, target, version);
}

auto trim(std::string_view sv) -> std::string_view;

inline auto parse_headers(auto it, auto end) -> std::vector<Header>
{
    auto headers = std::vector<Header>{};

    while (it != end)
    {
        const auto line = std::string_view{*it};
        if (line.empty())
            break;

        const auto separator = line.find(':');
        if (separator == std::string_view::npos)
        {
            return headers;
        }

        const auto key = line.substr(0, separator);
        const auto value = trim(line.substr(separator + 1));

        headers.push_back({key, value});
        ++it;
    }

    return headers;
}
}

struct Request
{
    std::string_view Method;
    std::string_view Target;
    std::string_view Version;
    std::vector<Header> Headers;
    std::string_view Body;

    [[nodiscard]] static auto from_raw(std::string_view raw) -> std::optional<Request>;
};

struct Response
{
    int Status;
    std::vector<Header> Headers;
    std::string Body;

    auto build_response() const -> std::string;
};

template <typename T>
inline constexpr bool is_string_like =
    std::constructible_from<std::string, std::remove_cvref_t<T>> || std::constructible_from<std::string_view, std::remove_cvref_t<T>>;

template <typename T>
    requires(!is_string_like<T>)
inline auto to_json(const T& obj) -> std::string
{
    std::string out = "{";
    bool first = true;
    constexpr auto ctx = std::meta::access_context::unchecked();
    template for (constexpr auto member : std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx)))
    {
        if (!first)
            out += ',';
        first = false;
        out += std::format("{}:{}", std::meta::identifier_of(member), obj.[:member:]);
    }
    out += '}';
    return out;
}

template <typename T>
inline auto serialize(T&& value) -> std::string
{
    using Type = std::remove_cvref_t<T>;

    if constexpr (requires { to_json(value); })
    {
        return to_json(value);
    }
    else if constexpr (std::constructible_from<std::string, T>)
    {
        return std::string{std::forward<T>(value)};
    }
    else if constexpr (std::constructible_from<std::string_view, T>)
    {
        return std::string{std::string_view{std::forward<T>(value)}};
    }
    else
    {
        static_assert(false, "Type cannot be serialized");
    }
}

template <typename T>
inline auto ok(const T& resp) -> Response
{
    return {200, {}, serialize(resp)};
}

template <typename T>
inline auto error(int code, const T& resp) -> Response
{
    return {code, {}, serialize(resp)};
}

template <FixedString Path>
auto view() -> Response
{
    static const auto body = [] {
        auto file = std::ifstream{
            std::string{std::string_view{Path}},
            std::ios::binary
        };

        if (!file)
            throw std::runtime_error{
                "Failed to open file: " + std::string{std::string_view{Path}}
            };

        return std::string{
            std::istreambuf_iterator<char>{file},
            std::istreambuf_iterator<char>{}
        };
    }();

    return ok(body);
}
}

template <>
struct std::formatter<wow::Header> : std::formatter<std::string_view>
{
    auto format(const wow::Header& h, std::format_context& ctx) const
    {
        return std::formatter<std::string_view>::format(std::format("{}: {}", h.Key, h.Value), ctx);
    }

};
