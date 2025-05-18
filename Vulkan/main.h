#pragma once

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
#define GLM_FORCE_RADIANS // Make sure glm is using radians as the argument unit in the library definition
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES // For non-nested structures, makes sure all types are aligned according to Vulkan/SPIR-V specification https://docs.vulkan.org/guide/latest/shader_memory_layout.html
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // Makes sure depth values cannot go outside [0,1] range. By default GLM uses OpenGL's convention of [-1,1]

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "stb_image.h"
#include <chrono>

#define HELIUM_VERTEX_BUFFERS
#define HELIUM_LOAD_MODEL
// #define HELIUM_DEBUG_LOG_FRAMES

class HelloTriangleApplication{

public:
    void run() ;

private:
    // Const params
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;
    const int MAX_FRAMES_IN_FLIGHT = 3;

    const std::string MODEL_PATH = "/Users/kambo/Helium/GameDev/Projects/CGSamples/Vulkan/objects/viking_room.obj";
    const std::string TEX_PATH = "/Users/kambo/Helium/GameDev/Projects/CGSamples/Vulkan/textures/viking_room.png";
    
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

    /*
    Contains bindings for:
    - MVP matrix
    - Main Texture
    */
    VkDescriptorSetLayout mainDescriptorSetLayout;

    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    uint32_t textureMipmaps;
    VkImage textureImageHandle;
    VkDeviceMemory textureImageDeviceMemory;
    VkImageView textureImageView;
    VkSampler textureSampler;

    VkImage depthPassImage;
    VkDeviceMemory depthPassMemory;
    VkImageView depthPassImageView;

    /*-
        We need multiple buffers as the mvp mat is updated each frame and we might have multiple frames in flight
        having only one would cause us to have a delay/run condition and a whole slew of issues.
    -*/
    std::vector<VkBuffer> mvpMatUniformBuffers;
    std::vector<VkDeviceMemory> mvpMatUniformBuffersMemory;
    std::vector<void*> mvpMatUniformBuffersMapHandles;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;


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
    void destroySwapChain();
    static void framebufferResizeCallback(GLFWwindow* window, int width, int height);
    void resetSwapChain();
    void recordCommandBuffer(VkCommandBuffer buffer, uint32_t swapchainImageIndex);
    void updateModelViewProj(uint32_t currentImage);
    
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
    void createSwapChainViews();
    void createRenderPass();
    void createPipeline();
    void createFramebuffers();
    void bufferCopy(VkBuffer src, VkBuffer dst, VkDeviceSize size);
    void createCommandPool();
    void createAndBindDeviceBuffer( VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage, VkMemoryPropertyFlags propertyFlags, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
    void createCommandBuffers();
    void createSyncObjects();

    #ifdef HELIUM_VERTEX_BUFFERS
    void createDeviceVertexBuffer();
    void createDescriptorSetLayout();
    void createDeviceIndexBuffer();
    void createCoherentUniformBuffers();
    void createDescriptorPool();
    void createAndBindDeviceImage(int width, int height, VkImage& imageDescriptor, VkDeviceMemory& imageMemory, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags, int mipmaps);
    void createTextureImage();
    void createTextureImageView();
    void createTextureSampler();
    void createDescriptorSets();
    void createDepthPassResources();
    VkFormat findFirstSupportedDepthFormatFromDefaults();
    VkFormat findFirstSupportedDepthFormat(const std::vector<VkFormat>& availableFormats, VkImageTiling depthTiling, VkFormatFeatureFlags features);
    #endif

    void convertImageLayout(VkImage srcImage, int mipmaps, VkFormat format, VkImageLayout srcLayout, VkImageLayout dstLayout);
    void bufferCopyToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t w, uint32_t h);
    VkCommandBuffer beginOneTimeCommands();
    void endAndSubmitOneTimeCommands(VkCommandBuffer tempBuffer);




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

    //-------------------------------image.cpp
    stbi_uc* loadImage(const char* path, int* width, int* height, int* channels);
    VkImageView createViewFor2DImage(VkImage image, int mipmaps, VkFormat format,VkImageAspectFlags imageAspect);
    void generatateImageMipMaps(VkImage image, VkFormat f, int32_t w, int32_t h, uint32_t levels);

    //-------------------------------model.cpp
    #ifdef HELIUM_LOAD_MODEL
    void loadModel();
    #endif

    //-------------------------------shaders.cpp
    VkShaderModule createShaderModule(const std::vector<char> binary);
};

// cmake --build /Users/kambo/Helium/GameDev/Projects/CGSamples/Vulkan/build --config Debug --target all -j 12 -v

//-------------------------------vertex.cpp
struct Vert{
    glm::vec3 pos;
    glm::vec3 col;
    glm::vec2 texCoords;
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescription();
    static VkVertexInputBindingDescription getBindingDescription();
};

#ifdef HELIUM_LOAD_MODEL
extern std::vector<Vert> vertices;
extern std::vector<uint32_t> indices;
#else
const std::vector<Vert> vertices = {
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

const std::vector<uint32_t> indices = {
    0, 1, 2,
    2, 3, 0,
    4, 5, 6,
    6, 7, 4
};
#endif

//-------------------------------PROJECTION RELATED STRUCTS
// Struct used as UBO (Uniform Buffer Object) binding in the vertex shader to apply projection
struct ModelViewProjection{
    glm::mat4 model;
    glm::mat4 view;
    glm::mat4 projection;
};