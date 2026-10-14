#pragma once

#include <memory>

template <typename T>
struct cow
{
    std::shared_ptr<T> _data;

    template <typename... Args>
    cow(Args... args)
        : _data{std::move(std::make_shared<T>(std::forward<Args>(args)...))}
    {
    }
};
