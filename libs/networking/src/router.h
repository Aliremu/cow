#pragma once

#include <format>
#include <functional>
#include <memory>
#include <meta>
#include <print>
#include <unordered_map>
#include <utility>
#include <vector>

#include "common.h"
#include "http.h"
#include "tcp.h"

namespace wow
{

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

struct string_hash
{
    using is_transparent = void;

    size_t operator()(std::string_view sv) const;
    size_t operator()(const std::string& str) const;
    size_t operator()(const char* ptr) const;
};

using Routers = std::unordered_map<std::string, std::function<Response()>, string_hash, std::equal_to<>>;

using ControllerPtr = std::unique_ptr<void, void (*)(void*)>;

namespace
{
// template <auto Type>
// consteval auto is_get_mapping() -> bool
// {
//     constexpr auto normalized = std::meta::remove_cvref(Type);
//     return std::meta::has_template_arguments(normalized) && std::meta::template_of(normalized) == ^^wow::get_mapping;
// }
//
// template <auto Type>
// consteval auto is_post_mapping() -> bool
// {
//     constexpr auto normalized = std::meta::remove_cvref(Type);
//     return std::meta::has_template_arguments(normalized) && std::meta::template_of(normalized) ==
//     ^^wow::post_mapping;
// }

template <auto Member>
consteval auto member_description()
{
    constexpr auto return_type = std::meta::return_type_of(Member);

    return std::pair{
        std::meta::display_string_of(Member),
        std::meta::display_string_of(return_type),
    };
}

template <auto Type, typename T>
auto bind(std::string_view base, T* controller, Routers& routes) -> void
{
    static constexpr auto ctx = std::meta::access_context::unchecked();
    template for (constexpr auto member : std::define_static_array(members_of(Type, ctx)))
    {
        if constexpr (!std::meta::is_special_member_function(member) && std::meta::is_function(member))
        {
            constexpr auto return_type = std::meta::return_type_of(member);
            constexpr auto description = member_description<member>();
            // constexpr auto is_get = is_get_mapping<return_type>();
            //
            // if constexpr (is_get_mapping<return_type>() || is_post_mapping<return_type>())
            // {
            //     using mapping_type = [:return_type:];
            //     std::println("Get[{}]: {} -> {}", is_get, mapping_type::path, description.second);
            // }

            constexpr auto annotations =
                std::define_static_array(std::meta::annotations_of_with_type(member, ^^wow::Mapping));
            constexpr auto size = annotations.size();
            if constexpr (size > 0)
            {
                constexpr auto mapping = std::meta::extract<wow::Mapping>(annotations[0]);
                std::println("Annotation: {}", std::string_view{mapping.Path});
                routes.emplace(
                    std::string{base} + std::string_view{mapping.Path},
                    std::function<Response()>([controller, method = &[:member:]] { return (controller->*method)(); }));
            }
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
        auto controller = std::make_unique<Type>();
        bind<^^Type, Type>(path, controller.get(), m_Routes);
        m_Controllers.emplace_back(controller.release(), [](void* ptr) { delete static_cast<Type*>(ptr); });
        std::println("Num routes: {}, Num controllers: {}", m_Routes.size(), m_Controllers.size());
        return *this;
    }

    template <typename Type>
    auto register_service() -> Router&
    {
        auto service = std::make_unique<Type>();
        m_Controllers.emplace_back(service.release(), [](void* ptr) { delete static_cast<Type*>(ptr); });
        return *this;
    }

    auto dispatch(const Request& request) -> Response;

  private:
    std::vector<ControllerPtr> m_Controllers;
    Routers m_Routes;
    static const Response RESPONSE_404;
};

auto serve(TcpListener& listener, Router& router) -> void;
}

template <std::size_t N>
struct std::formatter<wow::FixedString<N>> : std::formatter<std::string_view>
{
    auto format(const wow::FixedString<N>& s, std::format_context& ctx) const
    {
        return std::formatter<std::string_view>::format(std::format("{}", s.buf), ctx);
    }
};
