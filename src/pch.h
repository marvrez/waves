#pragma once

#include <volk.h>
#include <vk_mem_alloc.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <vector>
#include <array>
#include <string>
#include <functional>
#include <algorithm>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <memory>

template <typename T>
using Handle = std::shared_ptr<T>;

template<typename T, typename ...Args>
constexpr Handle<T> CreateHandle(Args&& ... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}