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
    std::cout<< "starting main loop" << std::endl;
    mainLoop();
    std::cout<< "ended main loop" << std::endl;
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
    std::cout<< "created logical device" << std::endl;
    createSwapChain();
    std::cout<< "created swap chain" << std::endl;
    createImageView();
    std::cout<< "created image view" << std::endl;
    createRenderPass();
    std::cout<< "created render pass" << std::endl;
    createPipeline();
    std::cout<< "created pipeline" << std::endl;
    createFramebuffers();
    std::cout<< "created frame buffers" << std::endl;
    createCommandPool();
    std::cout<< "created command pool" << std::endl;
    createCommandBuffers();
    std::cout<< "created command buffers" << std::endl;
    createSyncObjects();
    std::cout<< "created sync objects" << std::endl;
}


void HelloTriangleApplication::mainLoop() {
    std::cout<< "main loop" << std::endl;
    while( !glfwWindowShouldClose(window)){
        glfwPollEvents();
        std::cout << "call drawFrame()" << std::endl;
        drawFrame();
    }
}

void HelloTriangleApplication::cleanup() {
    vkDestroySemaphore(logiDevice, imageWriteableSemaphore, nullptr);
    vkDestroySemaphore(logiDevice, renderingFinishedSemaphore, nullptr);
    vkDestroyFence(logiDevice, frameFence, nullptr);
    vkDestroyCommandPool(logiDevice, commandPool, nullptr);
    for(auto fb : swapchainFramebuffers){
        vkDestroyFramebuffer(logiDevice, fb, nullptr);
    }
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

void HelloTriangleApplication::drawFrame(){
    std::cout << "FRAME:"<< frameCounter << std::endl;
    frameCounter++;
    std::cout << "waiting for frame" << std::endl;
    vkWaitForFences(logiDevice, 1, &frameFence, VK_TRUE, UINT64_MAX);
    if (vkResetFences(logiDevice, 1, &frameFence) != VK_SUCCESS){
        throw std::runtime_error("can't reset fence?");
    };
    std::cout << "fence reset :--" << frameFence << std::endl;

    uint32_t imageSwapchainIndex;
    vkAcquireNextImageKHR(logiDevice, swapChain, UINT64_MAX, imageWriteableSemaphore, VK_NULL_HANDLE, &imageSwapchainIndex);
    
    std::cout << "acquired image" << std::endl;
    vkResetCommandBuffer(graphicsCBuffer, 0);

    std::cout << "reset command buffer" << std::endl;
    recordCommandBuffer(graphicsCBuffer, imageSwapchainIndex);

    std::cout << "recorded command buffer" << std::endl;
    VkSubmitInfo commandSubmitInfo{};
    commandSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitedSemaphores[] = {imageWriteableSemaphore};
    // In what stage to wait for the specified semaphores
    VkPipelineStageFlags stagesToWaitOn[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    commandSubmitInfo.waitSemaphoreCount = 1;
    commandSubmitInfo.pWaitSemaphores = waitedSemaphores;
    commandSubmitInfo.pWaitDstStageMask = stagesToWaitOn;

    commandSubmitInfo.commandBufferCount = 1;
    commandSubmitInfo.pCommandBuffers = &graphicsCBuffer;

    VkSemaphore signaledSempahores[] = {renderingFinishedSemaphore};
    commandSubmitInfo.signalSemaphoreCount = 1;
    commandSubmitInfo.pSignalSemaphores = signaledSempahores;

    std::cout << "submitting to queue" << std::endl;
    std::cout << graphicsCommandQueue << std::endl;
    std::cout << &commandSubmitInfo << std::endl;
    std::cout << commandSubmitInfo.sType << std::endl;
    std::cout << frameFence << std::endl;
    if( vkQueueSubmit(graphicsCommandQueue, 1, &commandSubmitInfo, frameFence) != VK_SUCCESS){
        throw std::runtime_error("failed to submit commands to queue");
    }
    std::cout << "submitted to queue" << std::endl;

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signaledSempahores;

    VkSwapchainKHR presentSwapChains[]  = {swapChain};
    
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = presentSwapChains;
    presentInfo.pImageIndices = &imageSwapchainIndex;

    // Not needed here because 1 swapchain => result = result from vkQueuePresentKHR
    // presentInfo.pResults = nullptr; // Used to pass an array of VkResult for running multiple swapchain presentations.


    std::cout << "presenting" << std::endl;
    if (vkQueuePresentKHR(presentCommandQueue, &presentInfo) != VK_SUCCESS){
        throw std::runtime_error("failed to present command queue");
    }
    std::cout << "presented" << std::endl;
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