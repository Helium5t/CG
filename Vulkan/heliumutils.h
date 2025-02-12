#ifndef HELIUM_UTILS
#define HELIUM_UTILS

#include <string>
#define GLFW_INCLUDE_VULKAN // tells glfw to add vulkan api
#define VK_KHRONOS_VALIDATION_VALIDATE_BEST_PRACTICES true
#define VK_VALIDATION_VALIDATE_BEST_PRACTICES true
#define VK_VALIDATE_BEST_PRACTICES true
#define VK_KHRONOS_VALIDATION_VALIDATE_BEST_PRACTICES_ARM  true
#define VK_VALIDATION_VALIDATE_BEST_PRACTICES_ARM  true
#define VK_VALIDATE_BEST_PRACTICES_ARM true

#include <GLFW/glfw3.h>

inline const char* VkResultToString(VkResult r) {
    switch (r) {
        case VK_SUCCESS: return "SUCCESS";
        case VK_NOT_READY: return "NOT_READY";
        case VK_TIMEOUT: return "TIMEOUT";
        case VK_EVENT_SET: return "EVENT_SET";
        case VK_EVENT_RESET: return "EVENT_RESET";
        case VK_INCOMPLETE: return "INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY: return "ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED: return "ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST: return "ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED: return "ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT: return "ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT: return "ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT: return "ERROR_FEATURE_NOT_PRESENT";
        // Might happen for MacOS (check https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Instance#page_Encountered-VK_ERROR_INCOMPATIBLE_DRIVER)
        case VK_ERROR_INCOMPATIBLE_DRIVER: return "ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS: return "ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED: return "ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL: return "ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN: return "ERROR_UNKNOWN";
        default: return "UNKNOWN_RESULT";
    }
}

inline const char* VkDebugMessageTypeToString(VkDebugUtilsMessageTypeFlagsEXT r) {
    switch (r) {
        case VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT: return "GENERAL";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT: return "PERFORMANCE";
        case VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT: return "VALIDATION";

        default: return "UNKNOWN_TYPE";
    }
}

#endif