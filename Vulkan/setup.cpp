#include "main.h"


void HelloTriangleApplication::createInstance(){
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

void HelloTriangleApplication::setupDebugMessenger(){
    if(!validationLayerEnabled) return;
    VkDebugUtilsMessengerCreateInfoEXT mcInfo;
    fillCreateInfoForDebugHandler(mcInfo);
    mcInfo.pfnUserCallback = parseDebugCallbackLoop; /* To check if this actually works later on*/
    if (CreateDebugMessengerExtension(instance /* <- Debug messengers are specific to instances and layers, so we need this */, &mcInfo, nullptr, &debugCallbackHandler) != VK_SUCCESS) {
        throw std::runtime_error("failed to set up debug messenger");
    }
}


std::vector<const char*> HelloTriangleApplication::getRequiredExtensions(){
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


// Builds the vulkan representation of the used GPU
void HelloTriangleApplication::setPhysicalDevice(){
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        throw std::runtime_error("No Vulkan Supported GPUs found");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
    std::multimap<int, VkPhysicalDevice> possibleDevices;
    for( const auto& device : devices){
        int supportScore = rateDeviceSupport(device);
        if(supportScore > 0 ){
            physGraphicDevice = device;
            possibleDevices.insert(std::make_pair(supportScore, device));
        }
    }
    if (possibleDevices.rbegin() -> first > 0 ){
        physGraphicDevice = possibleDevices.rbegin() -> second;
        std::cout << "Selecting " << physGraphicDevice << " with support score: " << possibleDevices.cbegin()->first << std::endl;
    } else {
        throw std::runtime_error("No device meets requirements");
    }
}

void HelloTriangleApplication::createLogicalDevice(){
    QueueFamilyIndices qfi = findRequiredQueueFamily(physGraphicDevice);
    if(!qfi.has_values()){
        throw std::runtime_error("Selected physical device should have required queues but some are missing.");
    }
    std::vector<VkDeviceQueueCreateInfo> queueCreationInfos{};
    std::set<uint32_t> familySet {
        qfi.graphicsFamilyIndex.value(),
        qfi.presentationFamilyIndex.value()
    }; // Allows to set creation info (and thus create a queue) only once per index in the family indices.
    // e.g. is the same family can support both graphics and presentation commands, no need to create two queues, just create one that will receive both.

    float priority = 1.0; // always required, needed to give priority weight to each queue during scheduling. (e.g. We want graphics commands to always have priority over compute commands)
    for (uint32_t queueFamilyIndex : familySet){
        VkDeviceQueueCreateInfo qci{};
        qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qci.queueFamilyIndex = queueFamilyIndex;
        qci.queueCount = 1;
        qci.pQueuePriorities = &priority;
        queueCreationInfos.push_back(qci);
    }

    // What features of the physical device are being actually used.
    VkPhysicalDeviceFeatures usedPhysicalDeviceFeatures{}; // Empty for now cause we're doing nothing

    VkDeviceCreateInfo logicalDeviceCreationInfo{};
    logicalDeviceCreationInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    logicalDeviceCreationInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreationInfos.size()); 
    logicalDeviceCreationInfo.pQueueCreateInfos = queueCreationInfos.data();
    logicalDeviceCreationInfo.pEnabledFeatures = &usedPhysicalDeviceFeatures;

    logicalDeviceCreationInfo.enabledExtensionCount =static_cast<uint32_t>(requiredDeviceExtensionNames.size()); // No extensions needed atm.
    logicalDeviceCreationInfo.ppEnabledExtensionNames = requiredDeviceExtensionNames.data();

    // Actually this is ignored by most recent vulkan (https://docs.vulkan.org/spec/latest/chapters/extensions.html#extendingvulkan-layers-devicelayerdeprecation)
    // This is being filled in just for retrocompatibility.
    if(validationLayerEnabled){
        logicalDeviceCreationInfo.enabledLayerCount = static_cast<uint32_t>(validationLayerNames.size());
        logicalDeviceCreationInfo.ppEnabledLayerNames = validationLayerNames.data();
    }else{
        logicalDeviceCreationInfo.enabledLayerCount = 0;
    }
    VkResult logiDeviceCreationResult = vkCreateDevice(physGraphicDevice, &logicalDeviceCreationInfo, nullptr, &logiDevice);
    if( logiDeviceCreationResult != VK_SUCCESS){
        throw std::runtime_error(strcat("Failed to create logical device: " , VkResultToString(logiDeviceCreationResult)));
    }
    // These two calls may set the command queues to the same queue.
    vkGetDeviceQueue(
        logiDevice, 
        qfi.graphicsFamilyIndex.value(), 
        0, // Each queue family contains multiple queues in it, all those that support the features needed. This index referes to the position in the family of the queue to retrieve.
        &graphicsCommandQueue);
    vkGetDeviceQueue(logiDevice, qfi.presentationFamilyIndex.value(), 0, &presentCommandQueue);

}

void HelloTriangleApplication::setupRenderSurface(){
    VkResult createRenderSurfaceResult = glfwCreateWindowSurface(instance, window, nullptr, &renderSurface);
    if (createRenderSurfaceResult != VK_SUCCESS){
        throw std::runtime_error(strcat("Failed to create render surface: ", VkResultToString(createRenderSurfaceResult)));
    }
}

void HelloTriangleApplication::createSwapChain(){
    SwapChainSpecifications swapChainSpecs = checkSwapChainSpecifications(physGraphicDevice);

    VkSurfaceFormatKHR imageFormat = chooseImageFormat(swapChainSpecs.imageFormats);
    VkPresentModeKHR presentMode = choosePresentMode(swapChainSpecs.presentModes);
    VkExtent2D windowExtent = chooseWindowSize(swapChainSpecs.surfaceCapabilities);

    uint32_t frameCount = swapChainSpecs.surfaceCapabilities.minImageCount + 1; // +1 ensures we don't have to wait on the graphics driver.
    if (swapChainSpecs.surfaceCapabilities.maxImageCount > 0 && frameCount > swapChainSpecs.surfaceCapabilities.maxImageCount){
        frameCount = swapChainSpecs.surfaceCapabilities.maxImageCount;
    }
    VkSwapchainCreateInfoKHR swapchainCreateInfo{};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = renderSurface;
    swapchainCreateInfo.minImageCount = frameCount;
    swapchainCreateInfo.imageExtent = windowExtent;
    swapchainCreateInfo.imageFormat = imageFormat.format;
    swapchainCreateInfo.imageColorSpace = imageFormat.colorSpace;
    swapchainCreateInfo.imageArrayLayers = 1; // Used for VR, always 1 unless we need stereoscopic swap chain.
    /*
    Too many values : https://registry.khronos.org/vulkan/specs/latest/man/html/VkImageUsageFlagBits.html
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT means the swapchain generates images that can be used for a VkImageView or a Color buffer (vs Depth buffer) in a VkFrameBuffer.
    */
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    QueueFamilyIndices qfi = findRequiredQueueFamily(physGraphicDevice);
    uint32_t familyIndices[] = {qfi.graphicsFamilyIndex.value(), qfi.presentationFamilyIndex.value()};
    if(qfi.graphicsFamilyIndex != qfi.presentationFamilyIndex){ // We have two different queues for graphics and presentation and thus the chain will be shared among the two.
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = familyIndices;
    }else{ // Graphics and Presentation queue is the same queue, so no sharing needed.
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0; // Optional when exclusive
        swapchainCreateInfo.pQueueFamilyIndices = nullptr; // Optional when exclusive
    }
    swapchainCreateInfo.preTransform = swapChainSpecs.surfaceCapabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha =  VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE; // Used when recreating the swapchain for a new surface (e.g. change in window size)

    VkResult swapChainCreationResult = vkCreateSwapchainKHR(logiDevice, &swapchainCreateInfo, nullptr, &swapChain);
    if (swapChainCreationResult != VK_SUCCESS){
        throw std::runtime_error("could not create swapchain");
    }
    selectedSwapChainFormat = imageFormat.format;
    selectedSwapChainWindowSize = windowExtent;
    vkGetSwapchainImagesKHR(logiDevice, swapChain, &frameCount, nullptr);
    swapChainImages.resize(frameCount);
    vkGetSwapchainImagesKHR(logiDevice, swapChain, &frameCount, swapChainImages.data());
}

void HelloTriangleApplication::createImageView(){
    swapChainImageViews.resize(swapChainImages.size());
    for (int i =0; i< swapChainImages.size(); i++){
        VkImageViewCreateInfo viewCreationInfo{};
        viewCreationInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCreationInfo.format = selectedSwapChainFormat;
        viewCreationInfo.image = swapChainImages[0];
        viewCreationInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; // We will be passing 2D textures (you can also pass 1D and 3D textures)
        /*
        Swizzle allows you to, intuitively, swizzle the source channels around. 
        IDENTITY    : Channel is the same as the source (e.g. imageView.r = sourceImage.r)
        ZERO        : Channel is always 0
        ONE         : Channel is always 1 (e.g. if all image view channel are 1 the image will always be white)  
        R           : Map channel to red channel    (imageView.x = sourceImage.r)
        G           : Map channel to green channel  (e.g. imageView.r = sourceImage.g)
        B           : Map channel to blue channel
        A           : Map channel to alpha channel
        */
        viewCreationInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreationInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreationInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCreationInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        viewCreationInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        // Multiple layers can be useful for steroscopic apps (VR) and have each eye map to a layer
        viewCreationInfo.subresourceRange.baseArrayLayer = 0; // Only one layer, layer 0
        viewCreationInfo.subresourceRange.baseMipLevel = 0; // No mipmaps 
        viewCreationInfo.subresourceRange.layerCount = 1;
        viewCreationInfo.subresourceRange.levelCount = 1;

        VkResult viewCreationResult = vkCreateImageView(
            logiDevice,
            &viewCreationInfo,
            nullptr,
            &swapChainImageViews[0]
        );
        if (viewCreationResult != VK_SUCCESS){
            throw std::runtime_error(strcat("Failed to create image view:", VkResultToString(viewCreationResult)));
        }
    }
}

void HelloTriangleApplication::createPipeline(){
    std::vector<char> vShaderBinary = readFile("shaders/helloTriangle_v.spv");
    std::vector<char> fShaderBinary = readFile("shaders/helloTriangle_f.spv");

    VkShaderModule vShader = createShaderModule(vShaderBinary);
    VkShaderModule fShader = createShaderModule(fShaderBinary);

    VkPipelineShaderStageCreateInfo vShaderCreationInfo{};
    vShaderCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    // create vertex shader
    // Too many values https://registry.khronos.org/vulkan/specs/latest/man/html/VkShaderStageFlagBits.html
    // Fundamentally there is one for each possible stage, compute included. All general purpose stages are there
    // included platform specific (huawei) and raytracing stuff.
    vShaderCreationInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vShaderCreationInfo.module = vShader;
    // Name of the entry point name (first function to run in the shader)
    vShaderCreationInfo.pName = "main";
    vShaderCreationInfo.pSpecializationInfo = nullptr; // This allows you to pass constants at compile time
    // e.g. NORMAL_MAPPING_ENABLED so that the spir compiler will delete code paths based on the constants.

    VkPipelineShaderStageCreateInfo fShaderCreationInfo{};
    fShaderCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fShaderCreationInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fShaderCreationInfo.module = fShader;
    fShaderCreationInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {
        vShaderCreationInfo,
        fShaderCreationInfo
    };


    // Some data can be actually changed at runtime, but it's very limited (e.g. viewport size)
    std::vector<VkDynamicState> dynStates = {
        // The scissor is the rectangle where you can render that does not impact the coordinates
        // Ref: https://learn.microsoft.com/en-us/windows/win32/direct3d9/scissor-test
        // e.g. If you are rendering the rear view mirror in a car, the scissor will be the size of the mirror
        VK_DYNAMIC_STATE_SCISSOR,
        // The rectangle responsible for computing the pixel coordinates of the frame buffer from the device coordinates
        // The rectangle used for transforming coordinates.
        VK_DYNAMIC_STATE_VIEWPORT
    };

    VkPipelineDynamicStateCreateInfo dynStateCreationInfo{};
    dynStateCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynStateCreationInfo.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
    dynStateCreationInfo.pDynamicStates = dynStates.data();

    // Defines the way vertex data will be input 
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    // Defines the span of data and the way it is defined.
    // e.g.:    Each vertex data(normals, tans etc..) is 8 bytes long and
    //          is defined per-instance/per-vertex.
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
    // VA are usually normals, tangents etc... here the vertex is already in the shader so no need
    // to pass anything.
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreationInfo{};
    inputAssemblyCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssemblyCreationInfo.







    // graphics pipeline has been compiled, so cleanup shaders etc...
    vkDestroyShaderModule(logiDevice, vShader, nullptr);
    vkDestroyShaderModule(logiDevice, fShader, nullptr);
}