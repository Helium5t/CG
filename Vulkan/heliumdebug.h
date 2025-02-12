#ifndef HELIUM_DEBUG
#define HELIUM_DEBUG

#include "heliumutils.h"
#include <iostream>
#include <stdexcept>

/*
Check .cpp file for all explanaory comments
*/

static VKAPI_ATTR VkBool32 VKAPI_CALL parseDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT sev,
    VkDebugUtilsMessageTypeFlagsEXT msgType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
);

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
#endif