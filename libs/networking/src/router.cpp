#include "router.h"
#include "http.h"

namespace wow
{
auto string_hash::operator()(std::string_view sv) const -> size_t
{
    return std::hash<std::string_view>{}(sv);
}

auto string_hash::operator()(const std::string& str) const -> size_t
{
    return std::hash<std::string>{}(str);
}

auto string_hash::operator()(const char* ptr) const -> size_t
{
    return std::hash<std::string_view>{}(ptr);
}

const Response Router::RESPONSE_404 = {404, {}, "Not found"};

auto Router::dispatch(const Request& request) -> Response
{
    for (const auto& route : m_Routes)
    {
        if (route.first == request.Target || request.Target.starts_with(route.first + "/"))
            return route.second();
    }

    if (const auto& route = m_Routes.find("/"); route != m_Routes.end())
        return route->second();

    return RESPONSE_404;
}
}
