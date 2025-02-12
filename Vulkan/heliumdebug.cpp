#include "heliumdebug.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL parseDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT sev,
    VkDebugUtilsMessageTypeFlagsEXT msgType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
){
    std::cerr<<"debug callback with msg: "<< pCallbackData->pMessage << std::endl;
        /*
        Message types
        GENERAL     : Unrelated to the specification or performance
        VALIDATION  : Violation of the specification or a possible PEBCAK
        PERFORMANCE : Potential non-optimal use of Vulkan
        */
        std::cerr<<"type is: " << msgType << std::endl;
    // Filtering can also be done when creating the debug callback information in the createinfo struct
    // via the .severity field 
    if (sev >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT){
        std::cerr << "THIS IS SERIOUS" << std::endl;
    }
    return VK_FALSE;
}

// Get a pointer to the vkCreateDebugUtilsMessengerEXT function in order to create the debug messenger handler. This needs to be done as this function is not automatically loaded,
// it being an extension (EXT)
VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}