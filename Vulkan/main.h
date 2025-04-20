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

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN // tells glfw to add vulkan api
#endif
#include <GLFW/glfw3.h>
// linear algebra library 
#include <glm/glm.hpp>

#define HELIUM_VERTEX_BUFFERS
// #define HELIUM_DEBUG_LOG_FRAMES

class HelloTriangleApplication{

public:
    void run() ;

private:
    // Const params
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
    const int MAX_FRAMES_IN_FLIGHT = 3;
    
    const std::vector<const char*> validationLayerNames = {
        // Here the name has been removed because this validation layer crashes creation of frame buffer.
        // Somehow I am getting more debugging info compared to before, so no need in fixing this for now.
        // TODO: figure out wtf is going on. Probably has to do with vulkan .json configs for validation layers.
    // "VK_LAYER_KHRONOS_validation" // Standard bundle of validation layers included in LunarG SDK
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

    VkFormat selectedSwapChainFormat;
    VkExtent2D selectedSwapChainWindowSize;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    
    VkPipelineLayout pipelineLayout;
    VkRenderPass renderPass;
    VkPipeline gPipeline; 

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    // Each image in the swap chain should have a framebuffer associated to it.
    std::vector<VkFramebuffer> swapchainFramebuffers;

    // All commands (drawing, memory transfer etc..) 
    // are first submitted to a command pool
    // this way vulkan can better schedule them and dispatch them in groups.
    // This relieves duty of optimization of draw calls etc.. a bit and 
    // allows for dispatching commands from multiple threads.
    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> graphicsCBuffers;

    std::vector<VkSemaphore> imageWriteableSemaphores;
    std::vector<VkSemaphore> renderingFinishedSemaphores;
    std::vector<VkFence> frameFences;

    bool frameBufferResized = false;

    uint32_t currentFrame = 0;

    uint32_t frameCounter  = 0;



    //-------------------------------main.cpp

    void initWindow();
    void initVulkan() ;
    void mainLoop() ;
    void drawFrame();
    void cleanup();

    //-------------------------------sync.cpp
    void createSyncObjects();
    void destroySwapChain();
    void resetSwapChain();
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);

    //-------------------------------validation.cpp

    bool checkValidationLayerSupport();

    //-------------------------------setup.cpp
    
    uint32_t getFirstUsableMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags propertyFlags);
    std::vector<const char*> getRequiredExtensions();
    void createInstance();
    void setupDebugMessenger();
    void setPhysicalDevice();
    void createLogicalDevice();
    void setupRenderSurface();
    void createSwapChain();
    void createImageView();
    void createRenderPass();
    void createPipeline();
    void createFramebuffers();
    void createCommandPool();
    void createDeviceVertexBuffer();
    void createCommandBuffers();
    void recordCommandBuffer(VkCommandBuffer buffer, uint32_t swapchainImageIndex);


    //-------------------------------device_specs.cpp

    bool rateDeviceSupport(VkPhysicalDevice vkpd);

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

    bool supportsRequiredDeviceExtensions(VkPhysicalDevice vkpd);
    VkExtent2D chooseWindowSize(const VkSurfaceCapabilitiesKHR& capabilities);
    VkSurfaceFormatKHR chooseImageFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    VkPresentModeKHR choosePresentMode(const std::vector<VkPresentModeKHR>& modes);

    //-------------------------------device_specs.cpp
    
    void fillCreateInfoForDebugHandler(VkDebugUtilsMessengerCreateInfoEXT& toBeFilled);

    //-------------------------------shaders.cpp
    VkShaderModule createShaderModule(const std::vector<char> binary);
};

//-------------------------------vertex.cpp
struct Vert{
    glm::vec2 pos;
    glm::vec3 col;
    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescription();
    static VkVertexInputBindingDescription getBindingDescription();
};

const std::vector<Vert> vertices = {
    {{0.0f, -0.5f}, {1.0f, 1.0f, 1.0f}},
    {{0.5f, 0.5f}, {0.0f, 1.0f, 0.0f}},
    {{-0.5f, 0.5f}, {0.0f, 0.0f, 1.0f}}
};
#endif