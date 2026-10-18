#pragma once

#include <format>
#include <meta>
#include <optional>
#include <print>
#include <ranges>
#include <string_view>
#include <tuple>
#include <vector>

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

inline auto trim(std::string_view sv) -> std::string_view
{
    const auto whitespace = " \t\n\r\f\v";

    // Find first non-whitespace character and shift start forward
    const auto start = sv.find_first_not_of(whitespace);
    if (start == std::string_view::npos)
    {
        sv = {}; // String is entirely whitespace
        return sv;
    }
    sv.remove_prefix(start);

    // Find last non-whitespace character and shrink size backward
    const auto end = sv.find_last_not_of(whitespace);
    sv.remove_suffix(sv.size() - (end + 1));

    return sv;
}

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

    [[nodiscard]] static auto from_raw(std::string_view raw) -> std::optional<Request>
    {
        using namespace std::string_view_literals;
        constexpr auto header_body_separator = "\r\n\r\n"sv;

        const auto separator = raw.find(header_body_separator);
        if (separator == std::string_view::npos)
            return std::nullopt;

        const auto head = raw.substr(0, separator);
        const auto body = raw.substr(separator + header_body_separator.size());

        auto lines = head | std::views::split("\r\n"sv);
        auto it = lines.begin();
        if (it == lines.end())
            return std::nullopt;

        const auto request_line = detail::parse_request_line(*it);
        if (!request_line)
            return std::nullopt;

        const auto [method, target, version] = *request_line;
        ++it;

        const auto headers = detail::parse_headers(it, lines.end());

        return std::make_optional<Request>(method, target, version, headers, body);
    }
};

struct Response
{
    int Code;
    std::string Body;
};

template <typename T>
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
inline auto ok(const T& resp) -> Response
{
    return {200, std::to_string(resp)};
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
