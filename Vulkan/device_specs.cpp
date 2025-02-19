#include "main.h"

bool HelloTriangleApplication::rateDeviceSupport(VkPhysicalDevice vkpd){
    /*
    Keeping this here for reference on how to retrieve properties and features (and diff between the two).

    int score = 0;
    VkPhysicalDeviceProperties properties; // Name, type etc..
    vkGetPhysicalDeviceProperties(vkpd, &properties);
    VkPhysicalDeviceFeatures features; // Supported technologies, hardware accel, vr support etc... 
    vkGetPhysicalDeviceFeatures(vkpd, &features);
    std::cout << "rating GPU " << properties.deviceName << " is of type:"<< VkDeviceTypeToString(properties.deviceType) << std::endl;
    std::cout << "\tgeometry shader support: "<< std::boolalpha <<  static_cast<bool>(features.geometryShader) << std::endl;
    */

    QueueFamilyIndices familyIndex = findRequiredQueueFamily(vkpd);
    if(!familyIndex.has_values()){
        return false;
    }
    if (!supportsRequiredDeviceExtensions(vkpd)){
        return false;
    }
    SwapChainSpecifications swapChainSpecs = checkSwapChainSpecifications(vkpd);
    if (swapChainSpecs.presentModes.empty() || swapChainSpecs.imageFormats.empty()){
        return false;
    }
    return true;
}


HelloTriangleApplication::SwapChainSpecifications HelloTriangleApplication::checkSwapChainSpecifications(VkPhysicalDevice vkpd){
    SwapChainSpecifications scSpecs{};

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkpd, renderSurface, &scSpecs.surfaceCapabilities);

    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(vkpd, renderSurface, &formatCount, nullptr);
    if(formatCount != 0){
        scSpecs.imageFormats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(vkpd, renderSurface, &formatCount, scSpecs.imageFormats.data());
    }
    uint32_t presentModeCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(vkpd, renderSurface, &presentModeCount, nullptr);
    if(presentModeCount != 0){
        scSpecs.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(vkpd, renderSurface, &presentModeCount, scSpecs.presentModes.data());
    }
    return scSpecs;
}

HelloTriangleApplication::QueueFamilyIndices HelloTriangleApplication::findRequiredQueueFamily(VkPhysicalDevice vkpd){
    QueueFamilyIndices qfi;
    uint32_t familyCount;
    vkGetPhysicalDeviceQueueFamilyProperties(vkpd, &familyCount, nullptr);
    if (familyCount == 0){
        throw std::runtime_error("No queue families available.");
    }
    std::vector<VkQueueFamilyProperties> families(familyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(vkpd, &familyCount, families.data());
    int familyIndex = 0;
    for (const auto& family : families){
        /*
        Available bits for the queue. https://registry.khronos.org/vulkan/specs/latest/man/html/VkQueueFlagBits.html
        GRAPHICS_BIT            : Graphics operations
        COMPUTE_BIT             : Compute Operations
        TRANSFER_BIT            : Memory Transfer Operations
        SPARSE_BINDING_BIT      : Sparse Memory Operations (https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#sparsememory)
        PROTECTED_BIT           : Allows Memory protection (https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#memory-protected-memory)
        VIDEO_DECODE_BIT_KHR    : Video Decoding Ops
        VIDEO_ENCODE_BIT_KHR    : Video Encoding Ops
        OPTICAL_FLOW_BIT_NV     : Optical flow operations (Operations for Computer Vision, flow as in the "flow" between two frames of a video of a moving object.)
            */
        if (family.queueFlags & VK_QUEUE_GRAPHICS_BIT){ // Find first family to support graphic command queueing.
            qfi.graphicsFamilyIndex = familyIndex;
        }
        uint32_t surfacePresentationSupport = 0;
        vkGetPhysicalDeviceSurfaceSupportKHR(vkpd, familyIndex, renderSurface, &surfacePresentationSupport);
        if (surfacePresentationSupport){ // same as xxx != 0
            qfi.presentationFamilyIndex = familyIndex;
        }
        if(qfi.has_values()){
            break;
        }
        familyIndex++;
    }
    return qfi;
}

bool HelloTriangleApplication::supportsRequiredDeviceExtensions(VkPhysicalDevice vkpd){
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(vkpd, nullptr, &extensionCount, nullptr);
    if (extensionCount == 0){
        throw std::runtime_error("no device extensions supported");
    };
    std::vector<VkExtensionProperties> extensionProperties(extensionCount);
    vkEnumerateDeviceExtensionProperties(vkpd, nullptr, &extensionCount, extensionProperties.data());
    std::set<std::string> requiredExtensions(requiredDeviceExtensionNames.begin(), requiredDeviceExtensionNames.end());
    for(const auto& ext: extensionProperties){
        requiredExtensions.erase(ext.extensionName);
    };
    return requiredExtensions.empty();
}



VkExtent2D HelloTriangleApplication::chooseWindowSize(const VkSurfaceCapabilitiesKHR& capabilities){
    /*
        currentExtent is the current width and height of the surface, 
        or the special value (0xFFFFFFFF, 0xFFFFFFFF) indicating that the size will be adapted
        based on the swapchain that is targeting the surface.
        (i.e. if the swap chain is passing 1920x1080 frames, the size of the window will adapt to fit that size)
    */
    if(capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()/* Same as 0xFFFFFFFF*/){
        return capabilities.currentExtent;
    }
    int w, h;
    /* 
    We cannot reuse the previous values (WIDTH, HEIGHT) because in some cases (e.g. Apple Retina display lul) there is a difference between
        - the hardware pixel, as in the smallest rgb light emitting unit
        - the "logical" pixel, as in the number of pixels declared by the machine/OS to GLFW and the ones used by GLFW for positioning and sizing of the window. 
    GLFW uses "logical" pixels for deciding the window size, but vulkan instead uses the hardware ones, so the maxExtent might be a lot more than the logical maximum amount, given
    you can have a higher pixel count monitor display a lower resolution, this is often the case with retina since they purposefully display a lower resolution to implement image sharpening
    techniques to make everything look nicer. It's ultimately just a higher resolution screen using fancy antialiasing on a lower resolution image.
    Moreover, text size is often impacted by the logical resolution, so using a lower one makes the text bigger.

    In the tutorial, logical pixels are referred to as screen coordinates, which to me was extremely confusing given also the term of screen space coordinates, that go from 0 to 1
    irrespective of the amount of pixels.
    */
    glfwGetFramebufferSize(window, &w, &h); 
    VkExtent2D extent = {
        static_cast<uint32_t>(w),
        static_cast<uint32_t>(h)
    };

    // Clamp size to not overshoot max surface size
    extent.width = std::clamp(extent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    extent.height = std::clamp(extent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return extent;
}

VkSurfaceFormatKHR HelloTriangleApplication::chooseImageFormat(const std::vector<VkSurfaceFormatKHR>& formats){
    for(const auto& f:formats){
        if (f.format == VK_FORMAT_B8G8R8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            return f;
        }
    }
    return formats[0];
}

VkPresentModeKHR HelloTriangleApplication::choosePresentMode(const std::vector<VkPresentModeKHR>& modes){
    /*
    Present modes available
        IMMEDIATE :                 Present image as soon as it is submitted by application (may cause tearing due to refresh rate and app being out of sync)
        FIFO :                      Similar to VSync enabled, application fills a queue that is popped at each refresh of the screen (called a vertical blank).
        FIFO_RELAXED :              Similar to previous one, but if the application is late and the queue is empty, switch to immediate while the queue is empty. (Causes tearing when empty)
        MAILBOX  :                  Known as "triple buffering" (although not necessarily three frames in buffer). Same as FIFO but when the queue is full replace frames with newer ones. 
        
        Provided by the VK_KHR_shared_presentable_image extension
        SHARED_DEMAND_REFRESH :     app and presenter share the image pointer and app has to REQUEST an update, while presenter ignores the presentation. May cause tearing if out of sync.
        SHARED_CONTINUOUS_REFRESH : Same as previous, but the refresh request must only be sent once, can still cause tearing.

        Provided by VK_EXT_present_mode_fifo_latest_ready
        FIFO_LATEST_READY :         Same as FIFO but instead of dequeing one request, the presenter will deque the latest image (if they have a timestamp attached it will deque the one closest to the present but smaller than it.)
    */
    for(const auto& m:modes){
        if(m == VK_PRESENT_MODE_MAILBOX_KHR){
            return m;
        }
    }
    return VK_PRESENT_MODE_FIFO_KHR; // FIFO is always present
}
