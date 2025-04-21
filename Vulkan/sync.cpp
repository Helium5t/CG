#include "main.h"


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


void HelloTriangleApplication::recordCommandBuffer(VkCommandBuffer buffer, uint32_t swapchainImageIndex){
    VkCommandBufferBeginInfo bufferBeginInfo{};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    /*
        VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT: The command buffer will be rerecorded right after executing it once.
        VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT: This is a secondary command buffer that will be entirely within a single render pass.
        VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT: The command buffer can be resubmitted while it is also already pending execution
    */
    bufferBeginInfo.flags = 0;
    bufferBeginInfo.pInheritanceInfo = nullptr; // Do not inherit from any other begin info.

    // vkBeginCommandBuffer resets the command buffer everytime it is called.
    if (vkBeginCommandBuffer(buffer, &bufferBeginInfo) != VK_SUCCESS){
        throw std::runtime_error("failed to begin recording the command buffer");
    }

    /*-------------------------Render Pass Setup-----------------------------*/
    VkRenderPassBeginInfo renderPassBeginInfo{};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = renderPass;
    renderPassBeginInfo.framebuffer = swapchainFramebuffers[swapchainImageIndex];
    renderPassBeginInfo.renderArea.offset = {0, 0};
    renderPassBeginInfo.renderArea.extent = selectedSwapChainWindowSize;
    VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f }}}; // Clear frame to black
    renderPassBeginInfo.clearValueCount = 1;
    renderPassBeginInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(buffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    /*-------------------------Graphics Pipeline Binding-----------------------------*/
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gPipeline);
    // Setup of scissor and viewport as they are dynamic
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(selectedSwapChainWindowSize.width);
    viewport.height = static_cast<float>(selectedSwapChainWindowSize.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(buffer, 0,1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = selectedSwapChainWindowSize;
    vkCmdSetScissor(buffer, 0, 1, &scissor);

    #ifdef HELIUM_VERTEX_BUFFERS
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gPipeline);
    VkBuffer vertBuffers[]= {vertexBuffer};
    VkDeviceSize memoryOffsets[] = {0};
    vkCmdBindVertexBuffers(buffer, 0, 1, vertBuffers, memoryOffsets);
    vkCmdBindIndexBuffer(buffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    vkCmdDrawIndexed(buffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    #else
    vkCmdDraw(buffer, 3, 1, 0, 0);
    #endif

    vkCmdEndRenderPass(buffer);

    if (vkEndCommandBuffer(buffer) != VK_SUCCESS){
        throw std::runtime_error("failed to record the graphics command buffer");
    }
}

void HelloTriangleApplication::updateModelViewProj(uint32_t curFrameIndex){
    static std::chrono::steady_clock::time_point startTime = std::chrono::high_resolution_clock::now();

    std::chrono::steady_clock::time_point currentTime = std::chrono::high_resolution_clock::now();
    float timePassed = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

    ModelViewProjection mvp{};
    mvp.model = glm::rotate(glm::mat4(1.0f), timePassed * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    mvp.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    mvp.projection = glm::perspective(glm::radians(45.0f), selectedSwapChainWindowSize.width / (float) selectedSwapChainWindowSize.height, 0.1f, 10.0f);
    mvp.projection[1][1] *= -1; // clip coordinates are wrong in GLM. GLM uses y-up clip coordinates. 

    memcpy(mvpMatUniformBuffersMapHandles[curFrameIndex], &mvp, sizeof(mvp));
}