#pragma once

#include <utility>

template <typename T, typename... Args>
concept hasInit = requires(T obj, Args&&... args) {
    obj.Init(std::forward<Args>(args)...);
};

template <typename T>
concept hasClear = requires(T obj) {
    obj.Clear();
};
