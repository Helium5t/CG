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
    window = glfwCreateWindow(WIDTH,HEIGHT, "Helium Vulkan", nullptr, nullptr);
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, HelloTriangleApplication::framebufferResizeCallback);
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
    createDescriptorSetLayout();
    std::cout<< "created uniform buffer object bindings" << std::endl;
    createPipeline();
    std::cout<< "created pipeline" << std::endl;
    createFramebuffers();
    std::cout<< "created frame buffers" << std::endl;
    createCommandPool();
    std::cout << "created command pool" << std::endl;
    #ifdef HELIUM_VERTEX_BUFFERS
    createTextureImage();
    std::cout << "created texture image" << std::endl;
    createTextureImageView();
    std::cout << "created view for texture image" << std::endl;
    createTextureSampler();
    std::cout << "created texture sampler" << std::endl;
    createDeviceVertexBuffer();
    std::cout << "creates and bound vertex buffers" << std::endl;
    createDeviceIndexBuffer();
    std::cout << "created and bound index buffers" << std::endl;
    createCoherentUniformBuffers();
    std::cout << "created and bound uniform buffers" << std::endl;
    createDescriptorPool();
    std::cout << "created descriptor pool" << std::endl;
    createDescriptorSets();
    std::cout << "created descriptor sets" << std::endl;
    #endif
    createCommandBuffers();
    std::cout<< "created command buffers" << std::endl;
    createSyncObjects();
    std::cout<< "created sync objects" << std::endl;
}


void HelloTriangleApplication::mainLoop() {
    std::cout<< "main loop" << std::endl;
    while( !glfwWindowShouldClose(window)){
        glfwPollEvents();
        drawFrame();
    }
    vkDeviceWaitIdle(logiDevice);
}

void HelloTriangleApplication::cleanup() {
    destroySwapChain();

    vkDestroySampler(logiDevice, textureSampler, nullptr);
    vkDestroyImageView(logiDevice, textureImageView, nullptr);
    vkDestroyImage(logiDevice, textureImageHandle, nullptr);
    vkFreeMemory(logiDevice, textureImageDeviceMemory, nullptr);

    for (size_t i =0 ; i < mvpMatUniformBuffers.size(); i++){
        vkDestroyBuffer(logiDevice, mvpMatUniformBuffers[i], nullptr);
        vkFreeMemory(logiDevice, mvpMatUniformBuffersMemory[i], nullptr);
    }
    vkDestroyDescriptorPool(logiDevice, descriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(logiDevice, mainDescriptorSetLayout, nullptr);
    
    vkDestroyBuffer(logiDevice, vertexBuffer, nullptr);
    vkFreeMemory(logiDevice, vertexBufferMemory, nullptr);
    vkDestroyBuffer(logiDevice, indexBuffer, nullptr);
    vkFreeMemory(logiDevice, indexBufferMemory, nullptr);

    vkDestroyPipeline(logiDevice, gPipeline, nullptr);
    vkDestroyPipelineLayout(logiDevice, pipelineLayout, nullptr);
    
    vkDestroyRenderPass(logiDevice, renderPass, nullptr);
    for (int i =0 ; i < MAX_FRAMES_IN_FLIGHT; i++){
        vkDestroySemaphore(logiDevice, imageWriteableSemaphores[i], nullptr);
        vkDestroySemaphore(logiDevice, renderingFinishedSemaphores[i], nullptr);
        vkDestroyFence(logiDevice, frameFences[i], nullptr);
    }
    vkDestroyCommandPool(logiDevice, commandPool, nullptr);
    // In order : Device generating renders -> render surface -> instance -> window -> glfw.
    vkDestroyDevice(logiDevice, nullptr);
    if(validationLayerEnabled){
        DestroyDebugMessengerExtension(instance, debugCallbackHandler, nullptr); // Ideally this should be caught by the debug messenger when destroy is not called, and yet it doesn't happen
    }
    vkDestroySurfaceKHR(instance, renderSurface, nullptr);
    vkDestroyInstance(instance, nullptr/*Optional callback pointer*/);
    glfwDestroyWindow(window);
    glfwTerminate(); // Once this function is called, glfwInit(L#30) must be called again before using most GLFW functions. This deallocates everything GLFW related.
}

struct FrameLogger{
    template<typename T> FrameLogger& operator<<(const T& value){
        #ifdef HELIUM_DEBUG_LOG_FRAMES
        std::cout << value;
        #endif
        return *this;
    }
    
    FrameLogger& operator<<(std::ostream& (*manipulator)(std::ostream&)){
        #ifdef HELIUM_DEBUG_LOG_FRAMES
        std::cout << manipulator;
        #endif
        return *this;
    }
};

void emitFenceStatus(VkDevice device, VkFence* fence){
    FrameLogger flout;
    VkResult status = vkGetFenceStatus(device, *fence);
    flout << "Fence status: " << VkResultToString(status) << std::endl;
}

void HelloTriangleApplication::drawFrame(){
    #ifdef HELIUM_DO_NOT_REFRESH
    if(frameCounter > 0){
        return;
    }
    #endif
    FrameLogger flout;
    flout << "FRAME:"<< frameCounter << std::endl;
    flout << "waiting for frame" << std::endl;
    emitFenceStatus(logiDevice, &frameFences[currentFrame]);
    
    VkResult waitFencesResult =  vkWaitForFences(logiDevice, 1, &frameFences[currentFrame], VK_TRUE, UINT64_MAX);
    
    flout << "Wait fences result:------" << VkResultToString(waitFencesResult) << std::endl;
    flout << "fence signaled" << std::endl;
    
    updateModelViewProj(currentFrame);

    uint32_t imageSwapchainIndex;
    VkResult acquireImageResult = vkAcquireNextImageKHR(logiDevice, swapChain, UINT64_MAX, imageWriteableSemaphores[currentFrame], VK_NULL_HANDLE, &imageSwapchainIndex);

    flout<< "Acquire image result:------" << VkResultToString(acquireImageResult) << std::endl;
    flout << "acquired image, index is -- " << imageSwapchainIndex << std::endl;

    if (acquireImageResult == VK_ERROR_OUT_OF_DATE_KHR){
        resetSwapChain();
        return;
    }else if (acquireImageResult != VK_SUBOPTIMAL_KHR && acquireImageResult != VK_SUCCESS){
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    emitFenceStatus(logiDevice, &frameFences[currentFrame]);
    flout << "resetting fence" << std::endl;
    
    if (vkResetFences(logiDevice, 1, &frameFences[currentFrame]) != VK_SUCCESS){
        throw std::runtime_error("can't reset fence?");
    };
    
    flout << "fence reset" << std::endl;
    emitFenceStatus(logiDevice, &frameFences[currentFrame]);
    
    VkResult resetResult = vkResetCommandBuffer(graphicsCBuffers[currentFrame], /*VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT*/ 0);
    
    flout << "buffer reset result is:------"<<VkResultToString(resetResult)<< std::endl;
    flout << "reset command buffer" << std::endl;

    recordCommandBuffer(graphicsCBuffers[currentFrame], imageSwapchainIndex);

    flout << "recorded command buffer" << std::endl;
    
    VkSubmitInfo commandSubmitInfo{};
    commandSubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitedSemaphores[] = {imageWriteableSemaphores[currentFrame]};
    // In what stage to wait for the specified semaphores
    VkPipelineStageFlags stagesToWaitOn[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    commandSubmitInfo.waitSemaphoreCount = 1;
    commandSubmitInfo.pWaitSemaphores = waitedSemaphores;
    commandSubmitInfo.pWaitDstStageMask = stagesToWaitOn;

    commandSubmitInfo.commandBufferCount = 1;
    commandSubmitInfo.pCommandBuffers = &graphicsCBuffers[currentFrame];

    VkSemaphore signaledSempahores[] = {renderingFinishedSemaphores[currentFrame]};
    commandSubmitInfo.signalSemaphoreCount = 1;
    commandSubmitInfo.pSignalSemaphores = signaledSempahores;

    flout << "submitting to queue" << std::endl;
    emitFenceStatus(logiDevice, &frameFences[currentFrame]);
    
    if( vkQueueSubmit(graphicsCommandQueue, 1, &commandSubmitInfo, frameFences[currentFrame]) != VK_SUCCESS){
        throw std::runtime_error("failed to submit commands to queue");
    }
    
    flout << "submitted to queue" << std::endl;
    emitFenceStatus(logiDevice, &frameFences[currentFrame]);

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
    flout << "presenting" << std::endl;
    
    VkResult presentResult = vkQueuePresentKHR(presentCommandQueue, &presentInfo);
    if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || frameBufferResized){
        frameBufferResized = false;
        resetSwapChain();
    }
    else if (presentResult != VK_SUCCESS){
        throw std::runtime_error("failed to present command queue");
    }

    flout << "presented" << std::endl;
    frameCounter++;
    currentFrame = frameCounter%MAX_FRAMES_IN_FLIGHT;
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