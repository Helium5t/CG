#include "main.h"


void HelloTriangleApplication::fillCreateInfoForDebugHandler(VkDebugUtilsMessengerCreateInfoEXT& toBeFilled){
    std::cout<< "create info for debug handler" << std::endl;
    toBeFilled.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    toBeFilled.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT /*| VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT*/;
    toBeFilled.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_TOOL_PURPOSE_VALIDATION_BIT_EXT;
    toBeFilled.pfnUserCallback = parseDebugCallbackInstance;
    // This will be passed back to the debug handler when emitting the callback, this way you can access some data you want to emit into the debug callback 
    toBeFilled.pUserData = nullptr;
    // Added because I was getting a warning
    toBeFilled.flags = 0;
}
