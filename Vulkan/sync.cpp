#include "main.h"

void HelloTriangleApplication::createSyncObjects(){
    imageWriteableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderingFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    frameFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (int i = 0 ; i < MAX_FRAMES_IN_FLIGHT; i++){
        if(vkCreateSemaphore(logiDevice, &semaphoreCreateInfo, nullptr, &imageWriteableSemaphores[i]) != VK_SUCCESS){
            throw std::runtime_error("failed to create semaphore for signaling image is writeable");
        }
        if(vkCreateSemaphore(logiDevice, &semaphoreCreateInfo, nullptr, &renderingFinishedSemaphores[i]) != VK_SUCCESS){
            throw std::runtime_error("failed to create semaphore for signaling rendering has finished");
        }
        if(vkCreateFence(logiDevice, &fenceCreateInfo, nullptr, &frameFences[i]) != VK_SUCCESS){
            throw std::runtime_error("failed to create fence for signaling the frame is in flight");
        }
    }
}

void HelloTriangleApplication::destroySwapChain(){
    for(size_t i =0 ;  i < swapchainFramebuffers.size(); i++){
        vkDestroyFramebuffer(logiDevice, swapchainFramebuffers[i], nullptr);
    }
    for(size_t i =0; i < swapChainImageViews.size(); i++){
        vkDestroyImageView(logiDevice, swapChainImageViews[i], nullptr);
    }
    
    vkDestroySwapchainKHR(logiDevice, swapChain, nullptr);
}

void HelloTriangleApplication::resetSwapChain(){
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    while (w == 0 || h == 0 ){
        glfwGetFramebufferSize(window, &w, &h);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(logiDevice);

    destroySwapChain();

    createSwapChain();
    createImageView();
    createFramebuffers();
}


void HelloTriangleApplication::framebufferResizeCallback(GLFWwindow* window, int width, int height){
    HelloTriangleApplication* app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
    app->frameBufferResized = true;
}