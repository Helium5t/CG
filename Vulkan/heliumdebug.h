#ifndef HELIUM_DEBUG
#define HELIUM_DEBUG

#include "heliumutils.h"
#include <iostream>
#include <stdexcept>

/*
Check .cpp file for all explanaory comments
*/

/* Static enforces internal linkage (function declaration and definition can only be linked inside the same loading instance) 
    So we are forced to use inline here, alternative is declaring a non-static non-inline extern function and then assigning 
    the pointer in the .cpp  */
static inline VKAPI_ATTR VkBool32 VKAPI_CALL parseDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT sev,
    VkDebugUtilsMessageTypeFlagsEXT msgType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
){
    /*
    Message types
    GENERAL     : Unrelated to the specification or performance
    VALIDATION  : Violation of the specification or a possible PEBCAK
    PERFORMANCE : Potential non-optimal use of Vulkan
    */
    std::cerr<<"[DEBUG]["<< VkDebugMessageTypeToString(msgType)<<"]["<<sev<<"]: "<< pCallbackData->pMessage << std::endl;
    // Filtering can also be done when creating the debug callback information in the createinfo struct
    // via the .severity field 
    return VK_FALSE;

}

VkResult CreateDebugMessengerExtension(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
void DestroyDebugMessengerExtension(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);

#endif