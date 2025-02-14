// #include <vulkan/vulkan.h> // Provides funcs, structs and enums for vulkan
// replaces line 1 


#include <iostream>
#include <stdexcept>
#include <cstdlib> // Used for the two success statuses EXIT_SUCCESS and EXIT_FAILURE 
#include "heliumutils.h"
#include "heliumdebug.h"


class HelloTriangleApplication {
public:
    void run() {
        if (validationLayerEnabled){
            std::cout<< "VL Enabled" << std::endl;
        }else{
            std::cout<< "VL Disabled" << std::endl;
        }
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    // Const params
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    
    const std::vector<const char*> validationLayerNames = {
    "VK_LAYER_KHRONOS_validation" // Standard bundle of validation layers included in LunarG SDK
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

    void initWindow() {
        glfwInit();
        // All glfwWindowHint() set values that are retained up to the next glfwCreateContext() call
        // and are used to inform glfw how to create the context.

        // The default value for GLFW_CLIENT_API is GLFW_OPENGL_API but we are using vulkan
        // so tell it that no OpenGL API will be used, and thus no OpenGL context will be created.
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        // Disable resizing
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
        window = glfwCreateWindow(WIDTH,HEIGHT, "Helium Vulkan", nullptr, nullptr);

    }

    bool checkValidationLayerSupport(){
        uint32_t count;
        vkEnumerateInstanceLayerProperties(&count, nullptr);

        std::vector<VkLayerProperties> layers(count);
        vkEnumerateInstanceLayerProperties(&count, layers.data());
        for (const char* l : validationLayerNames){
            bool found = false;
            for (VkLayerProperties lp : layers){
                if(strcmp(l, lp.layerName) == 0){
                    found = true;
                    break;
                }
            }
            if (!found){
                return false;
            }
        }
        return true;
    }

    std::vector<const char*> getRequiredExtensions(){
        uint32_t glfwRequiredExtCount = 0;
        const char** glfwRequiredExtNames;
        glfwRequiredExtNames = glfwGetRequiredInstanceExtensions(&glfwRequiredExtCount); 
        std::vector<const char*> requiredExtNames;
        for(int i =0; i<glfwRequiredExtCount; i++){
            requiredExtNames.emplace_back(glfwRequiredExtNames[i]);
        }

        if (validationLayerEnabled){
            requiredExtNames.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }
        return requiredExtNames;
    }

    void createInstance(){
        #ifdef NDEBUG
            std::cout<< "Non Debug mode" << std::endl; 
        #endif
        if(validationLayerEnabled && !checkValidationLayerSupport()){
            throw std::runtime_error("specified validation layers are not supported");
        }

        VkApplicationInfo aInfo{};
        // All parameters here are optional, but allow for optimization by the vulkan library as it poses restrictions on what behaviour is expected of the application.
        aInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO; // defines structure type
        aInfo.pNext = nullptr; // Actually implied by initialization, added for clarity 
        aInfo.pApplicationName = "Triangle Ritual";
        aInfo.applicationVersion = VK_MAKE_API_VERSION(0,0,1337,0); // For dev versioning use, does not impact vulkan in any way.
        aInfo.pEngineName = "Helium Vulkan";
        aInfo.engineVersion = VK_MAKE_API_VERSION(0,0,1337,0); // For dev versioning use, does not impact vulkan in any way.
        aInfo.apiVersion = VK_API_VERSION_1_0; // Which version of the vulkan API to use. [ IMPACTS VULKAN CODE BEING RUN ]

        // Mandatory parameters, defines extensions and validation layers to be used
        // Vulkan can be extended by third party devs, these extensions add stuff 
        // such as:
        //  - acceleration structure (https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-acceleration-structure/introduction.html) support (for ray tracing optimizations) 
        // all of them can be found at https://registry.khronos.org/vulkan/
        // Validation layers are just layers that verify data is being formatted and passed around correctly, it is a set of calls 
        // added between Vulkan API Layer and the Graphics driver layer, allows easier debugging. (https://vulkan-tutorial.com/Overview#page_Validation-layers)
        VkInstanceCreateInfo iInfo{};
        iInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        iInfo.pApplicationInfo = &aInfo;
        // As vulkan is platform aognistic, we need an extension to add GLFW interfaces and bindings
        std::vector<const char*> glfwRequiredExtNames = getRequiredExtensions(); 
        iInfo.enabledExtensionCount = static_cast<uint32_t>(glfwRequiredExtNames.size());

        // Extra changes for macOS compatibility :) 
        iInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR; //macOS req

        std::vector<const char*> requiredExtNames = glfwRequiredExtNames;
        iInfo.enabledExtensionCount ++;
        requiredExtNames.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME); // macOS req
        // end of extra changes

        iInfo.ppEnabledExtensionNames = requiredExtNames.data();

        uint32_t supportedExtensionCount = 0;
        // Get just the number
        vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, nullptr);
        std::vector<VkExtensionProperties> supportedExtensions(supportedExtensionCount);
        // Get the full list
        vkEnumerateInstanceExtensionProperties(nullptr, &supportedExtensionCount, supportedExtensions.data());

        VkDebugUtilsMessengerCreateInfoEXT createInstanceDebuggerCI{};
        if(validationLayerEnabled){
            iInfo.enabledLayerCount = static_cast<uint32_t>(validationLayerNames.size());
            iInfo.ppEnabledLayerNames = validationLayerNames.data();

            fillCreateInfoForDebugHandler(createInstanceDebuggerCI);
            
            iInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &createInstanceDebuggerCI; 
        }else{
            iInfo.enabledLayerCount = 0;
            iInfo.pNext = nullptr;
        }

        VkResult res = vkCreateInstance(&iInfo, nullptr, &instance);
        std::cout << "Instance creation result: "<< VkResultToString(res) << std::endl;
        if (res != VK_SUCCESS){
            throw std::runtime_error("instance creation failure");
        }
    }


    void initVulkan() {
        createInstance();
        setupDebugMessenger();
    }

    void fillCreateInfoForDebugHandler(VkDebugUtilsMessengerCreateInfoEXT& toBeFilled){
        std::cout<< "create info for debug handler" << std::endl;
        toBeFilled.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        toBeFilled.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT  | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        toBeFilled.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_TOOL_PURPOSE_VALIDATION_BIT_EXT;
        toBeFilled.pfnUserCallback = parseDebugCallbackInstance;
        // This will be passed back to the debug handler when emitting the callback, this way you can access some data you want to emit into the debug callback 
        toBeFilled.pUserData = nullptr;
        // Added because I was getting a warning
        toBeFilled.flags = 0;
    }

    void setupDebugMessenger(){
        if(!validationLayerEnabled) return;
        VkDebugUtilsMessengerCreateInfoEXT mcInfo;
        fillCreateInfoForDebugHandler(mcInfo);
        mcInfo.pfnUserCallback = parseDebugCallbackLoop; /* To check if this actually works later on*/
        if (CreateDebugMessengerExtension(instance /* <- Debug messengers are specific to instances and layers, so we need this */, &mcInfo, nullptr, &debugCallbackHandler) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger");
        }
    }

    void mainLoop() {
        while( !glfwWindowShouldClose(window)){
            glfwPollEvents();
        }
    }

    void cleanup() {
        if(validationLayerEnabled){
            DestroyDebugMessengerExtension(instance, debugCallbackHandler, nullptr); // Ideally this should be caught by the debug messenger when destroy is not called, and yet it doesn't happen
        }

        vkDestroyInstance(instance, nullptr/*Optional callback pointer*/);
        glfwDestroyWindow(window);
        glfwTerminate(); // Once this function is called, glfwInit(L#30) must be called again before using most GLFW functions. This deallocates everything GLFW related.
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        std::cout << "hello" << std::endl;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}