#include "http.h"

namespace wow
{
auto detail::trim(std::string_view sv) -> std::string_view
{
    constexpr auto whitespace = " \t\n\r\f\v";

    const auto start = sv.find_first_not_of(whitespace);
    if (start == std::string_view::npos)
    {
        return {};
    }
    sv.remove_prefix(start);

    const auto end = sv.find_last_not_of(whitespace);
    sv.remove_suffix(sv.size() - (end + 1));
    return sv;
}

auto Request::from_raw(std::string_view raw) -> std::optional<Request>
{
    using namespace std::string_view_literals;
    constexpr auto header_body_separator = "\r\n\r\n"sv;

    const auto separator = raw.find(header_body_separator);
    if (separator == std::string_view::npos)
    {
        return std::nullopt;
    }

    const auto head = raw.substr(0, separator);
    const auto body = raw.substr(separator + header_body_separator.size());

    auto lines = head | std::views::split("\r\n"sv);
    auto it = lines.begin();
    if (it == lines.end())
    {
        return std::nullopt;
    }

    const auto request_line = detail::parse_request_line(*it);
    if (!request_line)
    {
        return std::nullopt;
    }

    const auto [method, target, version] = *request_line;
    ++it;
    const auto headers = detail::parse_headers(it, lines.end());

    return Request{method, target, version, headers, body};
}

auto Response::build_response() const -> std::string
{
    auto response = std::format("HTTP/1.1 {} OK\r\n", Status);
    for (const auto& header : Headers)
    {
        response += std::format("{}: {}\r\n", header.Key, header.Value);
    }
    response += std::format("Content-Length: {}\r\n", Body.size());
    response += "\r\n";
    response += Body;
    return response;
}
}