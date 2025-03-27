#include "main.h"

void HelloTriangleApplication::createSyncObjects(){
    VkSemaphoreCreateInfo semaphoreCreateInfo{};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceCreateInfo{};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    if(vkCreateSemaphore(logiDevice, &semaphoreCreateInfo, nullptr, &imageWriteableSemaphore) != VK_SUCCESS){
        throw std::runtime_error("failed to create semaphore for signaling image is writeable");
    }
    if(vkCreateSemaphore(logiDevice, &semaphoreCreateInfo, nullptr, &renderingFinishedSemaphore) != VK_SUCCESS){
        throw std::runtime_error("failed to create semaphore for signaling rendering has finished");
    }
    if(vkCreateFence(logiDevice, &fenceCreateInfo, nullptr, &frameFence) != VK_SUCCESS){
        throw std::runtime_error("failed to create fence for signaling the frame is in flight");
    }
}