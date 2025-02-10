// #include <vulkan/vulkan.h> // Provides funcs, structs and enums for vulkan
// replaces line 1 


#include <iostream>
#include <stdexcept>
#include <cstdlib> // Used for the two success statuses EXIT_SUCCESS and EXIT_FAILURE 
#include "heliumutils.h"

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

private:
    // Const params
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    // Objects
    GLFWwindow* window;
    VkInstance instance;

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

    void createInstance(){
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
        VkInstanceCreateInfo icInfo{};
        icInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        icInfo.pApplicationInfo = &aInfo;
        // As vulkan is platform aognistic, we need an extension to add GLFW interfaces and bindings
        uint32_t glfwRequiredExtCount = 0;
        const char** glfwRequiredExtNames;
        glfwRequiredExtNames = glfwGetRequiredInstanceExtensions(&glfwRequiredExtCount); 
        icInfo.enabledExtensionCount = glfwRequiredExtCount;
        // validation layers, unused atm
        icInfo.enabledLayerCount = 0;
        // unneeded, explicit clarity
        icInfo.pNext = nullptr;

        // Extra changes for macOS compatibility :) 
        icInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR; //macOS req

        std::vector<const char*> requiredExtNames;
        for(int i =0; i<icInfo.enabledExtensionCount; i++){
            requiredExtNames.emplace_back(glfwRequiredExtNames[i]);
        }
        icInfo.enabledExtensionCount ++;
        requiredExtNames.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME); // macOS req
        // end of extra changes

        icInfo.ppEnabledExtensionNames = requiredExtNames.data();
        VkResult res = vkCreateInstance(&icInfo, nullptr, &instance);
        std::cout << "Instance creation result:"<< VkResultToString(res) << std::endl;
        if (res != VK_SUCCESS){
            throw std::runtime_error("instance creation failure");
        }
    }


    void initVulkan() {
        createInstance();
    }

    void mainLoop() {
        while( !glfwWindowShouldClose(window)){
            glfwPollEvents();
        }
    }

    void cleanup() {
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