#pragma once

#include <algorithm>
#include <cstddef>
#include <string_view>

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
}
