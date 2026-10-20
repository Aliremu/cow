#pragma once

#include <memory>
#include <utility>

template <typename T>
struct cow
{
    std::shared_ptr<T> _data;

    template <typename... Args>
    cow(Args... args)
        : _data{std::move(std::make_shared<T>(std::forward<Args>(args)...))}
    {
    }

    cow(std::shared_ptr<T> ptr)
        : _data{std::move(ptr)}
    {
    }

    cow(const cow& other) = default;
    cow& operator=(const cow& other) = default;

    cow(cow&& other) noexcept = default;
    cow& operator=(cow&& other) noexcept = default;

    ~cow() = default;

    [[nodiscard]] auto clone() -> cow<T>
    {
        return std::make_shared<T>(*_data);
    }

    auto operator*(this const auto&& self) -> decltype(auto)
    {
        return std::forward_like<decltype(self)>(self._data).get();
    }
    auto operator->(this const auto&& self) -> decltype(auto)
    {
        return std::forward_like<decltype(self)>(self._data).get();
    }

    auto operator*(this auto&& self) -> T&
    {
        if (!self._data.unique())
            self._data = std::move(self.clone()._data);
        return *self._data;
    }
    auto operator->(this auto&& self) -> T*
    {
        if (!self._data.unique())
            self._data = std::move(self.clone()._data);
        return self._data.get();
    }
};
