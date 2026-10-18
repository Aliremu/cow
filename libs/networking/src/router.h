#pragma once

#include <format>
#include <functional>
#include <meta>
#include <print>
#include <unordered_map>
#include <utility>

#include "http.h"

namespace wow
{
template <std::size_t Capacity = 128>
struct FixedString
{
    char buf[Capacity]{};
    std::size_t size{};

    template <std::size_t N>
    constexpr FixedString(char const (&str)[N])
        : size{N - 1}
    {
        static_assert(N <= Capacity);
        std::copy(str, str + N, buf);
    }

    constexpr operator std::string_view() const
    {
        return {buf, size};
    }

    auto operator<=>(const FixedString&) const = default;
};

struct response
{
    int code;
};

template <FixedString Path = "", typename Response = void>
struct get_mapping : response
{
    static constexpr auto path = Path;
};

template <FixedString Path = "", typename Respond = void>
struct post_mapping : response
{
    static constexpr auto path = Path;
};

enum class Method : uint8_t
{
    Get = 0,
    Post,
    Put,
};

struct Mapping
{
    FixedString<> Path{""};
    Method test{Method::Get};
};

namespace
{
template <auto Type>
consteval auto is_get_mapping() -> bool
{
    constexpr auto normalized = std::meta::remove_cvref(Type);
    return std::meta::has_template_arguments(normalized) && std::meta::template_of(normalized) == ^^wow::get_mapping;
}

template <auto Type>
consteval auto is_post_mapping() -> bool
{
    constexpr auto normalized = std::meta::remove_cvref(Type);
    return std::meta::has_template_arguments(normalized) && std::meta::template_of(normalized) == ^^wow::post_mapping;
}

template <auto Member>
consteval auto member_description()
{
    constexpr auto return_type = std::meta::return_type_of(Member);

    return std::pair{
        std::meta::display_string_of(Member),
        std::meta::display_string_of(return_type),
    };
}

template <auto Type>
constexpr auto bind(std::string_view base, std::unordered_map<std::string, std::function_ref<Response()>>& routes)
    -> void
{
    static constexpr auto ctx = std::meta::access_context::unchecked();
    template for (constexpr auto member : std::define_static_array(members_of(Type, ctx)))
    {
        if constexpr (!std::meta::is_special_member_function(member))
        {
            constexpr auto return_type = std::meta::return_type_of(member);
            constexpr auto description = member_description<member>();
            constexpr auto is_get = is_get_mapping<return_type>();

            if constexpr (is_get_mapping<return_type>() || is_post_mapping<return_type>())
            {
                using mapping_type = [:return_type:];
                std::println("Get[{}]: {} -> {}", is_get, mapping_type::path, description.second);
            }
            std::println("{}", description.second);

            constexpr auto annotations =
                std::define_static_array(std::meta::annotations_of_with_type(member, ^^wow::Mapping));
            constexpr auto size = annotations.size();
            // std::println("Anootation: {}", size);
            if constexpr (size > 0)
            {
                constexpr auto mapping = std::meta::extract<wow::Mapping>(annotations[0]);
                std::println("Anootation: {}", std::string_view{mapping.Path});
                routes.emplace(std::string{base}, member);
            }
        }
    }
}
}

class Router
{
  public:
    template <auto Type>
    auto route(std::string_view path) -> Router&
    {
        bind<Type>(path, m_Routes);
        return *this;
    }

  private:
    std::unordered_map<std::string, std::function_ref<Response()>> m_Routes;
};
}

template <std::size_t N>
struct std::formatter<wow::FixedString<N>> : std::formatter<std::string_view>
{
    auto format(const wow::FixedString<N>& s, std::format_context& ctx) const
    {
        return std::formatter<std::string_view>::format(std::format("{}", s.buf), ctx);
    }
};
