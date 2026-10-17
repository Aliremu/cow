#pragma once

#include <format>
#include <meta>
#include <print>

namespace wow
{
template <std::size_t N>
struct FixedString
{
    char buf[N]{};
    constexpr FixedString(char const (&str)[N])
    {
        std::copy(str, str + N, buf);
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
constexpr auto bind() -> void
{
    static constexpr auto ctx = std::meta::access_context::unprivileged();
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
        }
    }
}
}

class Router
{
  public:
    template <typename Type>
    auto route(std::string_view path) -> Router&
    {
        bind<^^Type>();
        return *this;
    }

  private:
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
