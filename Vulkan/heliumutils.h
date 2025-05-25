#ifndef HELIUM_UTILS
#define HELIUM_UTILS

#include "helium_required.h"
#include <string>
#include <fstream>
#define VK_KHRONOS_VALIDATION_VALIDATE_BEST_PRACTICES true
#define VK_VALIDATION_VALIDATE_BEST_PRACTICES true
#define VK_VALIDATE_BEST_PRACTICES true
#define VK_KHRONOS_VALIDATION_VALIDATE_BEST_PRACTICES_ARM  true
#define VK_VALIDATION_VALIDATE_BEST_PRACTICES_ARM  true
#define VK_VALIDATE_BEST_PRACTICES_ARM true

#include <iostream>
#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN // tells glfw to add vulkan api
#endif
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

inline const char* VkDebugMessageSeverityToString(VkDebugUtilsMessageSeverityFlagBitsEXT r) {
    switch (r) {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: return "WARN";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: return "INFO";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT: return "VERB";
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: return "ERRR";

        default: return "UNKNOWN_SEV";
    }
}

inline const char* VkDeviceTypeToString(VkPhysicalDeviceType t){
    switch(t){
        case VK_PHYSICAL_DEVICE_TYPE_OTHER: return "OTHER";
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "INTEGRATED_GPU";
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU: return "DEDICATED_GPU";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU: return "VIRTUAL_GPU";
        case VK_PHYSICAL_DEVICE_TYPE_CPU: return "CPU";
        
        default: return "UNKNOWN_TYPE";
    }
}

// Used for reading shader binary files
static std::vector<char> readFile(const std::string& filename) {
    // std::ios::ate is same as doing open() and then seek(EOF)
    // This way we automatically know the size of the file by knowing the file head pos.
    std::ifstream file(filename, std::ios::ate | std::ios::binary); 

    if (!file.is_open()) {
        std::cout<< "could not open:" << filename << std::endl;
        throw std::runtime_error("failed to open file!");
    }

    size_t fSize = (size_t)file.tellg();
    std::vector<char> content(fSize);
    file.seekg(0);
    file.read(content.data(), fSize);

    file.close();
    return content;
}

#endif