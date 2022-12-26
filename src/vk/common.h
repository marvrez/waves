#pragma once

#define VK_CHECK(x) _VkCheck(__LINE__, __FILE__, (VkResult)x)

static void _VkCheck(int line, const char* file, VkResult x)
{
    const char* error;

    switch (x) {
        case (VK_SUCCESS): return;
        case (VK_NOT_READY): error = "VK_NOT_READY"; break;
        case (VK_TIMEOUT): error = "VK_TIMEOUT"; break;
        case (VK_EVENT_SET): error = "VK_EVENT_SET"; break;
        case (VK_INCOMPLETE): error = "VK_INCOMPLETE"; break;
        case (VK_ERROR_OUT_OF_HOST_MEMORY): error = "VK_ERROR_OUT_OF_HOST_MEMORY"; break;
        case (VK_ERROR_OUT_OF_DEVICE_MEMORY): error = "VK_ERROR_OUT_OF_DEVICE_MEMORY"; break;
        case (VK_ERROR_INITIALIZATION_FAILED): error = "VK_ERROR_INITIALIZATION_FAILED"; break;
        case (VK_ERROR_DEVICE_LOST): error = "VK_ERROR_DEVICE_LOST"; break;
        case (VK_ERROR_MEMORY_MAP_FAILED): error = "VK_ERROR_MEMORY_MAP_FAILED"; break;
        case (VK_ERROR_LAYER_NOT_PRESENT): error = "VK_ERROR_LAYER_NOT_PRESENT"; break;
        case (VK_ERROR_EXTENSION_NOT_PRESENT): error = "VK_ERROR_EXTENSION_NOT_PRESENT"; break;
        case (VK_ERROR_FEATURE_NOT_PRESENT): error = "VK_ERROR_FEATURE_NOT_PRESENT"; break;
        case (VK_ERROR_INCOMPATIBLE_DRIVER): error = "VK_ERROR_INCOMPATIBLE_DRIVER"; break;
        case (VK_ERROR_TOO_MANY_OBJECTS): error = "VK_ERROR_TOO_MANY_OBJECTS"; break;
        case (VK_ERROR_FORMAT_NOT_SUPPORTED): error = "VK_ERROR_FORMAT_NOT_SUPPORTED"; break;
        case (VK_ERROR_FRAGMENTED_POOL): error = "VK_ERROR_FRAGMENTED_POOL"; break;
        case (VK_ERROR_OUT_OF_POOL_MEMORY): error = "VK_ERROR_OUT_OF_POOL_MEMORY"; break;
        case (VK_ERROR_INVALID_EXTERNAL_HANDLE): error = "VK_ERROR_INVALID_EXTERNAL_HANDLE"; break;
        case (VK_ERROR_SURFACE_LOST_KHR): error = "VK_ERROR_SURFACE_LOST_KHR"; break;
        case (VK_ERROR_NATIVE_WINDOW_IN_USE_KHR): error = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR"; break;
        case (VK_SUBOPTIMAL_KHR): error = "VK_SUBOPTIMAL_KHR"; break;
        case (VK_ERROR_OUT_OF_DATE_KHR): error = "VK_ERROR_OUT_OF_DATE_KHR"; break;
        case (VK_ERROR_INCOMPATIBLE_DISPLAY_KHR): error = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR"; break;
        case (VK_ERROR_VALIDATION_FAILED_EXT): error = "VK_ERROR_VALIDATION_FAILED_EXT"; break;
        case (VK_ERROR_INVALID_SHADER_NV): error = "VK_ERROR_INVALID_SHADER_NV"; break;
        case (VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT): error = "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT"; break;
        case (VK_ERROR_FRAGMENTATION): error = "VK_ERROR_FRAGMENTATION"; break;
        case (VK_ERROR_NOT_PERMITTED_EXT): error = "VK_ERROR_NOT_PERMITTED_EXT"; break;
        case (VK_ERROR_INVALID_DEVICE_ADDRESS_EXT): error = "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT"; break;
        case (VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT): error = "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT"; break;
        default: error = "UNKNOWN"; break;
    }

    char buffer[512];
    snprintf(buffer, 512, "%s:%i: error: %i = %s", file, line, int(x), error);
    throw std::runtime_error(buffer);
}

// Helpers for robustly executing the two-call pattern
template <typename T, typename F, typename... Ts>
static inline auto GetVectorNoError(F&& f, Ts&&... ts)
{
    uint32_t count = 0;
    f(ts..., &count, nullptr);
    std::vector<T> results(count);
    f(ts..., &count, results.data());
    return results;
}
template <typename T, typename F, typename... Ts>
static inline auto GetVector(F&& f, Ts&&... ts)
{
    uint32_t count = 0;
    VK_CHECK(f(ts..., &count, nullptr));
    std::vector<T> results(count);
    VK_CHECK(f(ts..., &count, results.data()));
    return results;
}