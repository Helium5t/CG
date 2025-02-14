#include "heliumdebug.h"

// Gets a pointer to the vkCreateDebugUtilsMessengerEXT function in order to create the debug messenger handler. This needs to be done as this function is not automatically loaded,
// it being an extension (EXT)
VkResult CreateDebugMessengerExtension(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
    std::cout << "Creating DebugMessengerExtension" << std::endl;
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        // Actually create the debug messenger here
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugMessengerExtension(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator){
    std::cout << "Destroying DebugMessengerExtension" << std::endl;
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr){
        return func(instance, debugMessenger, pAllocator);
    }
}