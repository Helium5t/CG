#ifndef HELIUM_MAIN_H
#define HELIUM_MAIN_H

#include <iostream>
#include <stdexcept>
#include <cstdlib> // Used for the two success statuses EXIT_SUCCESS and EXIT_FAILURE 
#include <map>
#include <set>
#include "heliumutils.h"
#include "heliumdebug.h"
#include <optional>
// #include <cstdint> // Necessary for uint32_t
#include <limits> // Necessary for std::numeric_limits
#include <algorithm> // Necessary for std::clamp


class HelloTriangleApplication{

public:
    void run() ;

private:
    // Const params
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    
    const std::vector<const char*> validationLayerNames = {
    "VK_LAYER_KHRONOS_validation" // Standard bundle of validation layers included in LunarG SDK
    };

    const std::vector<const char*> requiredDeviceExtensionNames = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };

    #ifdef NDEBUG
    const bool validationLayerEnabled = false;
    #else
    const uint32_t validationLayerEnabled = true;
    #endif

    // Objects
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugCallbackHandler;
    VkSurfaceKHR renderSurface; // Where each render will be presented

    
    // Automatically destroyed by vulkan so no need to destroy it in cleanup
    VkPhysicalDevice physGraphicDevice = VK_NULL_HANDLE;  
    // The logical device (code interface for the physical device)
    VkDevice logiDevice; 
    VkQueue graphicsCommandQueue;
    VkQueue presentCommandQueue;
    VkSwapchainKHR swapChain;

    std::vector<VkImage> swapChainImages;
    VkFormat selectedSwapChainFormat;
    VkExtent2D selectedSwapChainWindowSize;

    void initWindow();

    bool checkValidationLayerSupport();

    std::vector<const char*> getRequiredExtensions();

    void createInstance();


    void initVulkan() ;

    void createSwapChain();

    void setupRenderSurface();

    void createLogicalDevice();

    // Builds the vulkan representation of the used GPU
    void setPhysicalDevice();

    struct SwapChainSpecifications {
        VkSurfaceCapabilitiesKHR surfaceCapabilities;
        std::vector<VkSurfaceFormatKHR> imageFormats;
        std::vector<VkPresentModeKHR> presentModes;
    };

    SwapChainSpecifications checkSwapChainSpecifications(VkPhysicalDevice vkpd);

    // Contains the index of the queue family we need. The index refers to the position in the array returned by vkGetPhysicalDeviceQueueFamilyProperties.
    struct QueueFamilyIndices{
        // Queue family for graphics commands
        std::optional<uint32_t> graphicsFamilyIndex; // 0 is a valid family index (each index represents a queue family that supports certain commands) so we need a way to discern between null and 0.
        // Queue family for presentation to surface
        std::optional<uint32_t> presentationFamilyIndex; 

        inline bool has_values(){
            return graphicsFamilyIndex.has_value() && presentationFamilyIndex.has_value();
        }
    };

    QueueFamilyIndices findRequiredQueueFamily(VkPhysicalDevice vkpd);

    bool rateDeviceSupport(VkPhysicalDevice vkpd);

    VkSurfaceFormatKHR chooseImageFormat(const std::vector<VkSurfaceFormatKHR>& formats);

    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes);

    VkExtent2D chooseWindowSize(const VkSurfaceCapabilitiesKHR& capabilities);

    bool supportsRequiredDeviceExtensions(VkPhysicalDevice vkpd);

    void fillCreateInfoForDebugHandler(VkDebugUtilsMessengerCreateInfoEXT& toBeFilled);

    void setupDebugMessenger();

    void mainLoop() ;

    void cleanup();

};
#endif