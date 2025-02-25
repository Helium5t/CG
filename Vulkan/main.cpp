// #include <vulkan/vulkan.h> // Provides funcs, structs and enums for vulkan
// replaces line 1 


// #include <iostream>
// #include <stdexcept>
// #include <cstdlib> // Used for the two success statuses EXIT_SUCCESS and EXIT_FAILURE 
// #include <map>
// #include <set>
// #include "heliumutils.h"
// #include "heliumdebug.h"
// #include <optional>
// // #include <cstdint> // Necessary for uint32_t
// #include <limits> // Necessary for std::numeric_limits
// #include <algorithm> // Necessary for std::clamp

#include "main.h"


void HelloTriangleApplication::run() {
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

void HelloTriangleApplication::initWindow() {
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


void HelloTriangleApplication::initVulkan() {
    createInstance();
    setupDebugMessenger();
    setupRenderSurface();
    setPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageView();
    createRenderPass();
    createPipeline();
}


void HelloTriangleApplication::mainLoop() {
    while( !glfwWindowShouldClose(window)){
        glfwPollEvents();
    }
}

void HelloTriangleApplication::cleanup() {
    vkDestroyPipeline(logiDevice, gPipeline, nullptr);
    vkDestroyPipelineLayout(logiDevice, pipelineLayout, nullptr);
    vkDestroyRenderPass(logiDevice, renderPass, nullptr);
    for(const auto& i: swapChainImageViews){
        vkDestroyImageView(logiDevice, i, nullptr);
    }
    vkDestroySwapchainKHR(logiDevice, swapChain, nullptr);
    // In order : Device generating renders -> render surface -> instance -> window -> glfw.
    vkDestroyDevice(logiDevice, nullptr);
    vkDestroySurfaceKHR(instance, renderSurface, nullptr);
    if(validationLayerEnabled){
        DestroyDebugMessengerExtension(instance, debugCallbackHandler, nullptr); // Ideally this should be caught by the debug messenger when destroy is not called, and yet it doesn't happen
    }
    vkDestroyInstance(instance, nullptr/*Optional callback pointer*/);
    glfwDestroyWindow(window);
    glfwTerminate(); // Once this function is called, glfwInit(L#30) must be called again before using most GLFW functions. This deallocates everything GLFW related.
}

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