#include "main.h"

#define HELIUM_PRINT_EXTENSIONS
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
    #ifdef HELIUM_PRINT_EXTENSIONS
    std::cout << "supported extensions" << std::endl;
    for (const auto& xt: supportedExtensions){
        std::cout << xt.extensionName << std::endl;
    }
    #endif

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
        // Trying to fix the damn bug
        requiredExtNames.push_back("VK_EXT_metal_surface");
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
        maxMsaaSupported = getMaxSamplesMSAA();
        std::cout << "Max MSAA samples supported: " << static_cast<uint32_t>(maxMsaaSupported) << std::endl;
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
    usedPhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE; // We need anisotropic filtering for the main texture.
    usedPhysicalDeviceFeatures.sampleRateShading = VK_TRUE; // Small optimization to improve antialiasing, by shading per sample instead of per fragment.
    // Given MSAA's nature, this will multiplicate effectively the amount of fragment shader runs.

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
    std::cout << "supported frame count is " << frameCount << std::endl;
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

void HelloTriangleApplication::createSwapChainViews(){
    swapChainImageViews.resize(swapChainImages.size());
    for (int i =0; i< swapChainImages.size(); i++){
        swapChainImageViews[i] = createViewFor2DImage(
            swapChainImages[i],
            1,
            selectedSwapChainFormat,
            VK_IMAGE_ASPECT_COLOR_BIT
        );
    }
}

void HelloTriangleApplication::createRenderPass(){
    VkAttachmentDescription mainColorAttachmentDescription{}; 
    mainColorAttachmentDescription.format = selectedSwapChainFormat; // Image format i.e. bits per channel and linear/gamma etc...
    mainColorAttachmentDescription.samples = maxMsaaSupported; // Max supported samples
    /*
    Defines what to do before the render pass
    LOAD : Preserve what is already there
    CLEAR : Clear all memory
    DONT_CARE : No need to preserve previous data or define behaviour, let underlying implementation do whatever it wants.
    NONE : The image is not accessed at all during the render pass, do nothing.
    */
    mainColorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clears the screen right before writing to frame buffer
    /*
    Defines what to do after the render pass, has same values as load except CLEAR*/
    mainColorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // stores the rendered frame into the frame buffer.
    // Previous ops were for color and depth buffers, all stencil buffers follow instead these two ops
    mainColorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; 
    mainColorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // Optimizes the layout of the pixels of the image based on usage
    /*
    Too many values, see https://registry.khronos.org/vulkan/specs/latest/man/html/VkImageLayout.html
    VK_IMAGE_LAYOUT_PRESENT_SRC_KHR means it's going to be used to be presented to screen.
    Other useful values:
    - VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL use as color attachment (attachments are descriptors of input and output resources for a render pass)
        e.g. : descriptor of render-target, of textures etc... 
            they will then provide the resources needed for the associated pixel in the render pass. 
            read more: https://stackoverflow.com/questions/46384007/what-is-the-meaning-of-attachment-when-speaking-about-the-vulkan-api
        fundamentally, they are i/o bindings for the render pass. 
    - VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL use as destination for a copy operation in memory
     */
    mainColorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // pre-pass
    mainColorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // This will be multisampled into the resolve attachment

    // Used to set the binding for the render pass
    VkAttachmentReference mainColorAttachmentRef{};
    mainColorAttachmentRef.attachment = 0; // reference to first attachment (we have only one atm)
    mainColorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Use as color attachment (output)


    VkAttachmentDescription depthAttachmentDescription{};
    depthAttachmentDescription.format = findFirstSupportedDepthFormatFromDefaults();
    depthAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    depthAttachmentDescription.samples = maxMsaaSupported;
    depthAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription msaaResultColorAttachmentDescription{};
    msaaResultColorAttachmentDescription.format = selectedSwapChainFormat;
    msaaResultColorAttachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT;
    msaaResultColorAttachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    msaaResultColorAttachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    msaaResultColorAttachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    msaaResultColorAttachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    msaaResultColorAttachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    msaaResultColorAttachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Actually the color attachment presented to screen.

    VkAttachmentReference msaaResultAttachmentRef{};
    msaaResultAttachmentRef.attachment = 2;
    msaaResultAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpassDesc{};
    /*
    Baisc bindings
    GRAPHICS : Graphics subpass
    COMPUTE  : Compute subpass (compute shaders)
    // Platform specific
    EXECUTION_GRAPH_AMDX : AMD specific feature,a SIMD format that defines a graph where shaders can run in parallel.
    // Needs extensions
    RAY_TRACING_KHR         : Ray tracing pass
    SUBPASS_SHADING_HUAWEI  : A shading subpass optimization specific tu huawei
    RAY_TRACING_NV          : Same as _KHR
*/
    subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; 
    subpassDesc.colorAttachmentCount = 1;
    subpassDesc.pColorAttachments = &mainColorAttachmentRef;
    subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;
    subpassDesc.pResolveAttachments = &msaaResultAttachmentRef;

    // Add dependency to make sure pipeline waits for the image to be writeable
    VkSubpassDependency dependency{};
    // Special bit, specified the pre-pipeline implicit pass or the post-pipeline implicit pass
    // respectively if it's in srcSubpass or dstSubpass.
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; 
    dependency.dstSubpass = 0;

    // If srcSubpass is equal to dstSubpass then the VkSubpassDependency does 
    // not directly define a dependency. Instead, it enables pipeline barriers 
    // to be used in a render pass instance within the identified subpass, 
    // where the scopes of one pipeline barrier must be a subset of those described
    // by one subpass dependency
    dependency.srcStageMask = 
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | 
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask = 0;

    dependency.dstStageMask = 
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT| 
        VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask =
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


    std::array<VkAttachmentDescription,3> descriptions = {
        mainColorAttachmentDescription,
        depthAttachmentDescription,
        msaaResultColorAttachmentDescription,
    };
    VkRenderPassCreateInfo renderPassCreationInfo{};
    renderPassCreationInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreationInfo.attachmentCount = static_cast<uint32_t>(descriptions.size());
    renderPassCreationInfo.pAttachments = descriptions.data();
    renderPassCreationInfo.subpassCount = 1;
    renderPassCreationInfo.pSubpasses = &subpassDesc;
    renderPassCreationInfo.dependencyCount = 1;
    renderPassCreationInfo.pDependencies = &dependency;

    if(vkCreateRenderPass(logiDevice, &renderPassCreationInfo, nullptr, &renderPass) != VK_SUCCESS){
        throw std::runtime_error("failed to create render pass");
    }

}

void HelloTriangleApplication::createPipeline(){
    #ifndef HELIUM_VERTEX_BUFFERS
    std::vector<char> vShaderBinary = readFile("/Users/kambo/Helium/GameDev/Projects/CGSamples/Vulkan/shaders/v1_helloTriangle.spv");
    std::vector<char> fShaderBinary = readFile("/Users/kambo/Helium/GameDev/Projects/CGSamples/Vulkan/shaders/f1_helloTriangle.spv");
    #else

    VkVertexInputBindingDescription bindingDescription = Vert::getBindingDescription();
    std::array<VkVertexInputAttributeDescription, 3> attributeDescription = Vert::getAttributeDescription();

    std::vector<char> vShaderBinary = readFile("/Users/kambo/Helium/GameDev/Projects/CGSamples/Vulkan/shaders/v3_mvpVertex.spv");
    std::vector<char> fShaderBinary = readFile("/Users/kambo/Helium/GameDev/Projects/CGSamples/Vulkan/shaders/f3_gammaCorrection.spv");
    #endif 

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
        // Explanatory image https://vulkan-tutorial.com/images/viewports_scissors.png
    };

    VkPipelineDynamicStateCreateInfo dynStateCreationInfo{};
    dynStateCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynStateCreationInfo.dynamicStateCount = static_cast<uint32_t>(dynStates.size());
    dynStateCreationInfo.pDynamicStates = dynStates.data();

    // Defines the way vertex data will be input 
    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo{};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
    #ifdef HELIUM_VERTEX_BUFFERS
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescription.size());
    vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescription.data();
    #else
    // Defines the span of data and the way it is defined.
    // e.g.:    Each vertex data(normals, tans etc..) is 8 bytes long and
    //          is defined per-instance/per-vertex.
    vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
    vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
    // VA are usually normals, tangents etc... here the vertex is already in the shader so no need
    // to pass anything.
    vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
    vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;
    #endif

    VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreationInfo{};
    inputAssemblyCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    /*
    POINT : Each vertex is standalone and creates a point, no face or edge is created (primitive_size =1 )
    LINE : Every two vertices create an edge, no faces (primitive_size = 2)
    TRIANGLE : Every three vertices create a triangle (primitive_size = 3)
    PATCH : An arbitrary number of vertices can create a face, used for tesselation (https://www.khronos.org/opengl/wiki/Tessellation#Patches)
    *_LIST (POINT, LINE, TRIANGLE, PATCH) : Each primitive is separate so 
        vertices_num = primitive_size * num_primitives
    *_STRIP (LINE, TRIANGLE) : consecutive primitives share vertices (lines one, triangles two), so 
        minimum vertices_num = 2 + (primitive_size - 1) * num_primitives (can be more if primitives are not all consecutive)
    *_FAN (TRIANGLE) : All primitives (just triangles) share a vertex, so 
        vertices_num = 2 + num_triangles
    *_WITH_ADJACENCY (LINE, TRIANGLE, LIST/STRIP): each primitive/set of primitives is also defined together with adjacent vertices,
                        vertices that are not part of the primitive that can be accessed in the geometry shader.  
                        e.g. :  STRIP_WITH_ADJACENCY means there will be x primitives defined (consecutive ones) + primitive_size * num_primitives adjacent vertices 
                                                            for LINE_STRIP you can only have 2 more adjacent vertices for each strip because lines do not allow "branching"
                                LIST_WITH_ADJACENCY means each primitive defined (no sharing) will have +primitive_size vertices for the adjacent ones
    */                  
    inputAssemblyCreationInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    // If VK_TRUE, strips can be broken up by setting an index to 0xFFFFFFFF 
    inputAssemblyCreationInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0;
    viewport.y = 0;
    viewport.width = (float) selectedSwapChainWindowSize.width;
    viewport.height = (float) selectedSwapChainWindowSize.height;
    viewport.maxDepth = 1.0f;
    viewport.minDepth = 0.0f;

    VkRect2D scissor{};
    scissor.extent = selectedSwapChainWindowSize;
    scissor.offset = {0, 0};

    VkPipelineViewportStateCreateInfo viewportStateCreationInfo{};
    viewportStateCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreationInfo.viewportCount = 1;
    // viewportStateCreationInfo.pViewports = &viewport; // Only needed if viewport is static  (not in VkDynamicState)
    viewportStateCreationInfo.scissorCount = 1; 
    // viewportStateCreationInfo.pScissors = &scissor; // Only needed if scissor is static  (not in VkDynamicState)


    VkPipelineRasterizationStateCreateInfo rasterizationStageCreationInfo{};
    rasterizationStageCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    // depthClamp enabled will clamp z to [0,1] instead of discarding values outside
    rasterizationStageCreationInfo.depthClampEnable = VK_FALSE; 
    // disable geometry data going through rasterizer, would basically disable output to the frame buffer.
    rasterizationStageCreationInfo.rasterizerDiscardEnable = VK_FALSE;
    /*
    FILL : Fill in the face with fragments
    LINE : Fill only edges with fragments (wireframe mode)
    POINT: Fill only vertices with fragments (point cloud basically)
    FILL_RECTANGLE_NV : Will generate fragments to fill the bounding rectangle of the vertices in a primitive.
            e.g. : Given n vertices (works for any polygon), it will find the smallest bounding box (view space) containing them 
                    and generate a fragments for each pixel inside it.
    
    Each mode that isn't fill requires enabling some GPU features
    */
    rasterizationStageCreationInfo.polygonMode = VK_POLYGON_MODE_FILL;
    // Each edge is wide 1 fragment, any value different from 1 requires GPU features enabled.
    rasterizationStageCreationInfo.lineWidth = 1.0f;
    rasterizationStageCreationInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    #ifndef HELIUM_VERTEX_BUFFERS
    rasterizationStageCreationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
    #else
    rasterizationStageCreationInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    #endif

    // should rasterizer add a constant bias to all z values?
    rasterizationStageCreationInfo.depthBiasEnable = VK_FALSE;
    // Values to generate the depth bias, can control based on a constant
    // or slope of the fragment, shadow mapping uses this to avoid self-shadowing
    rasterizationStageCreationInfo.depthBiasConstantFactor = 0.0f;
    rasterizationStageCreationInfo.depthBiasSlopeFactor = 0.0f;
    rasterizationStageCreationInfo.depthBiasClamp = 0.0f;

    // Multisampling does antialiasing by sampling fragment shaders from polygons that raster to the same fragment (which might be sub-pixel or overlap multiple pixels).
    // Allows for less expensive anti-aliasing vs Super-sampling (render at higher resolution and then downscale).
    // Requires GPU features for it enabled.
    VkPipelineMultisampleStateCreateInfo  multisamplingStageCreationInfo{};
    multisamplingStageCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

    // Enable running fragment shader once for each sample rather than each pixel => multiple times per pixel based on amount of overlapping geometry
    multisamplingStageCreationInfo.sampleShadingEnable = VK_TRUE; 
    multisamplingStageCreationInfo.rasterizationSamples = maxMsaaSupported;
    // In range [0,1], how many samples of the total number of samples in a pixel to actually shade.
    // 0 => only one sample => shade per pixel
    // 1 => all samples => shade per sample
    // in-between is a fraction of the sample, so given 10 samples,  0.5 will only actually shade half of them.
    multisamplingStageCreationInfo.minSampleShading = 1.0f;
    // A bitmask that says for each fragment if some samples should be ignored. (32 bits)
    // e.g. : (4 bits for simplicity) a geometry generates a mask with all 0010, if the mask for the entire screen is 1101
    //          those samples from the geometry will not contribute to the final fragent. This allows selection of specific fragments
    //          in different places of the frame buffer.
    multisamplingStageCreationInfo.pSampleMask = nullptr;
    // during sampling
    // temporarily set alpha to the coverage of first sample output available
    multisamplingStageCreationInfo.alphaToCoverageEnable = VK_FALSE;
    // temporarily set alpha to one 
    multisamplingStageCreationInfo.alphaToOneEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencilStageCreationInfo {};
    depthStencilStageCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilStageCreationInfo.depthTestEnable = VK_TRUE;
    depthStencilStageCreationInfo.depthWriteEnable = VK_TRUE;
    depthStencilStageCreationInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencilStageCreationInfo.depthBoundsTestEnable = VK_FALSE;
    depthStencilStageCreationInfo.minDepthBounds = 0.0f;
    depthStencilStageCreationInfo.maxDepthBounds = 1.0f;
    depthStencilStageCreationInfo.stencilTestEnable = VK_FALSE;
    depthStencilStageCreationInfo.front = {};
    depthStencilStageCreationInfo.back = {};

    VkPipelineColorBlendAttachmentState colorBlendAttachmentState{}; // Handles color blending per-buffer in the framebuffer
    // which channels to write
    colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachmentState.blendEnable = VK_FALSE; // do not blend (overwrite)
    // Types of blending, allows setting blending with the source, the destination and with constant values set globally by the pipeline stage 
    // https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#framebuffer-blendfactors
    colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; 
    colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; 
    colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;  // Additive color blending (actually none because blending is disabled and weight is fully on source)
    colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // How much source alpha contributes to final 
    colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // How much destination alpha contributes to final (prev value in buffer)
    colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD; // additive alpha blending
    
    VkPipelineColorBlendStateCreateInfo colorBlendingStageCreationInfo{}; // global settings for color blending
    colorBlendingStageCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlendingStageCreationInfo.logicOpEnable = VK_FALSE; // enables bitwise blending instead of mixing
    colorBlendingStageCreationInfo.logicOp = VK_LOGIC_OP_COPY; // Unused, it is used for bitwise operations (so VK_TRUE above)
    colorBlendingStageCreationInfo.attachmentCount = 1; 
    colorBlendingStageCreationInfo.pAttachments = &colorBlendAttachmentState;
    // The following fields are optional, and are used for determining how to compute the blending factor when
    // VK_BLEND_FACTOR is of type VK_BLEND_FACTOR_*_CONSTANT_*
    // e.g. if blendConstant[0] is 0.1, then VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR yields 0.1 for the Red channel.
    colorBlendingStageCreationInfo.blendConstants[0] = 0.0f; // R
    colorBlendingStageCreationInfo.blendConstants[1] = 0.0f; // G
    colorBlendingStageCreationInfo.blendConstants[2] = 0.0f; // B
    colorBlendingStageCreationInfo.blendConstants[3] = 0.0f; // A

    // This pipeline is used to create bindings for uniform variables, accessed by the shaders.
    VkPipelineLayoutCreateInfo pipelineLayoutCreationInfo{};
    pipelineLayoutCreationInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    // for explicity, these are not needed and are by default 0/null.
    pipelineLayoutCreationInfo.setLayoutCount = 1;
    pipelineLayoutCreationInfo.pSetLayouts = &mainDescriptorSetLayout;
    pipelineLayoutCreationInfo.pushConstantRangeCount = 0; // Number of push constant, an element that can be used to pass dynamic values to the shaders
    pipelineLayoutCreationInfo.pPushConstantRanges = nullptr;
    if(vkCreatePipelineLayout(logiDevice, &pipelineLayoutCreationInfo, nullptr, &pipelineLayout) != VK_SUCCESS){
        throw std::runtime_error("error when creating the pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineCreationInfo{};
    pipelineCreationInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreationInfo.stageCount = 2; // number of programmable shader stages: Fragment and Vertex
    pipelineCreationInfo.pStages = shaderStages;
    pipelineCreationInfo.pVertexInputState = &vertexInputCreateInfo;
    pipelineCreationInfo.pInputAssemblyState = &inputAssemblyCreationInfo;
    pipelineCreationInfo.pViewportState = &viewportStateCreationInfo;
    pipelineCreationInfo.pRasterizationState = &rasterizationStageCreationInfo;
    pipelineCreationInfo.pMultisampleState = &multisamplingStageCreationInfo;
    pipelineCreationInfo.pDepthStencilState = &depthStencilStageCreationInfo; 
    pipelineCreationInfo.pColorBlendState = &colorBlendingStageCreationInfo;
    pipelineCreationInfo.pDynamicState = &dynStateCreationInfo;

    pipelineCreationInfo.layout = pipelineLayout;
    pipelineCreationInfo.renderPass = renderPass;
    pipelineCreationInfo.subpass = 0; // first subpass is the entry point
    // Pipelines can be child of other pipelines in order to simplify definitions and optimize switching between similar pipelines
    // https://registry.khronos.org/vulkan/specs/latest/html/vkspec.html#pipelines-pipeline-derivatives
    pipelineCreationInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineCreationInfo.basePipelineIndex = -1;

    if(vkCreateGraphicsPipelines(logiDevice, VK_NULL_HANDLE, 1, &pipelineCreationInfo, nullptr, &gPipeline) != VK_SUCCESS){
        throw std::runtime_error("failed to create graphics pipeline");
    }


    // graphics pipeline has been compiled, so cleanup shaders etc...
    vkDestroyShaderModule(logiDevice, vShader, nullptr);
    vkDestroyShaderModule(logiDevice, fShader, nullptr);
}

void HelloTriangleApplication::createAndBindDeviceBuffer(
    VkDeviceSize bufferSize, 
    VkBufferUsageFlags bufferUsage,
    VkMemoryPropertyFlags propertyFlags,
    VkBuffer& buffer, 
    VkDeviceMemory& bufferMemory
){
    #ifndef HELIUM_VERTEX_BUFFERS
    return;
    #else

    VkBufferCreateInfo bufferCreationInfo{};
    bufferCreationInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreationInfo.size = bufferSize;
    bufferCreationInfo.usage = bufferUsage;
    /*
    Sharing mode is for the queue, not for the shaders. Only the graphics queue 
    will use the buffer so exclusive.
    */
   bufferCreationInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   
   if (vkCreateBuffer(logiDevice, &bufferCreationInfo, nullptr, &buffer) != VK_SUCCESS){
       throw std::runtime_error("failed to create the buffer object on the device");
    }
    
    /*----- Allocate memory -----*/
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(logiDevice, buffer, &memReqs);
    
    VkMemoryAllocateInfo allocationInfo{};
    allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocationInfo.allocationSize = memReqs.size;
    allocationInfo.memoryTypeIndex = getFirstUsableMemoryType(
        memReqs.memoryTypeBits, propertyFlags);
        
    if(vkAllocateMemory(logiDevice, &allocationInfo, nullptr, &bufferMemory) != VK_SUCCESS){
        throw std::runtime_error("failed to allocate buffer device memory for buffer object being created");
    }
    
    /*----- Bind allocated memory to vertex buffer object -----*/
    vkBindBufferMemory(logiDevice, buffer, bufferMemory, 0);
    #endif
}

#ifdef HELIUM_VERTEX_BUFFERS

bool hasStencilComponent(VkFormat format){
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

void HelloTriangleApplication::createDeviceIndexBuffer(){
    VkDeviceSize indexBufferSize = 
        sizeof(indices[0]) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkMemoryPropertyFlags stagingBufferMemProperties = 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    createAndBindDeviceBuffer(
        indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        stagingBufferMemProperties,
        stagingBuffer,
        stagingBufferMemory
    );

    void* bufferData;

    vkMapMemory(logiDevice, stagingBufferMemory, 0, indexBufferSize, 0, &bufferData);
    memcpy(bufferData, indices.data(), (size_t) indexBufferSize);
    vkUnmapMemory(logiDevice, stagingBufferMemory);

    createAndBindDeviceBuffer(indexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        indexBuffer,
        indexBufferMemory
    );

    bufferCopy(stagingBuffer, indexBuffer, indexBufferSize);

    vkDestroyBuffer(logiDevice, stagingBuffer, nullptr);
    vkFreeMemory(logiDevice, stagingBufferMemory, nullptr);


}

void HelloTriangleApplication::createDeviceVertexBuffer(){
    VkDeviceSize vertexBufferSize = 
        sizeof(vertices[0]) * vertices.size();
    std::cout << "size of vbuffer is " << vertexBufferSize  << "(" << sizeof(vertices[0]) << ")" << std::endl;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;

    VkMemoryPropertyFlags stagingBufferMemProperties = 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    createAndBindDeviceBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        stagingBufferMemProperties,
        stagingBuffer,
        stagingBufferMemory
    );
    

    /*----- Write to device buffer -----*/
    void* bufferData;
    // Get virtual address pointer to the device buffer
    vkMapMemory(logiDevice, stagingBufferMemory, 0, vertexBufferSize, 0, &bufferData);
    // Write to the memory via memcpy, copying the vertex buffer content into the device memory
    memcpy(bufferData, vertices.data(), (size_t) vertexBufferSize);
    vkUnmapMemory(logiDevice, stagingBufferMemory);


    VkMemoryPropertyFlags vertexBufferMemProperties = 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    createAndBindDeviceBuffer(
        vertexBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, 
        vertexBufferMemProperties,
        vertexBuffer, 
        vertexBufferMemory
    );

    bufferCopy(stagingBuffer, vertexBuffer, vertexBufferSize);

    vkDestroyBuffer(logiDevice, stagingBuffer, nullptr);
    vkFreeMemory(logiDevice, stagingBufferMemory, nullptr);

}

void HelloTriangleApplication::createDescriptorSetLayout(){

    VkDescriptorSetLayoutBinding mvpMatDescriptorBinding{};
    mvpMatDescriptorBinding.binding = 0; // index in the shader
    /*
    Too many values, check https://registry.khronos.org/vulkan/specs/latest/man/html/VkDescriptorType.html
    Describes the type of binding to have to the gpu. Notable values:
    - UNIFORM_BUFFER
    - INPUT_ATTACHMENT : associated to an image resource with its own view that can be used for local load operations from a framebuffer (it's a G buffer)
    - SAMPLED_IMAGE : associated to an image resource, with its own view where you can perform sampling operation. (A Texture sampler binding)
    */
    mvpMatDescriptorBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    mvpMatDescriptorBinding.descriptorCount = 1;
    mvpMatDescriptorBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    mvpMatDescriptorBinding.pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutBinding textureSamplerLayoutBinding{};
    textureSamplerLayoutBinding.binding = 1;
    textureSamplerLayoutBinding.descriptorCount = 1;
    textureSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    textureSamplerLayoutBinding.pImmutableSamplers = nullptr;
    textureSamplerLayoutBinding.stageFlags  = VK_SHADER_STAGE_FRAGMENT_BIT;

    std::array<VkDescriptorSetLayoutBinding,2> bindings = {
        mvpMatDescriptorBinding, textureSamplerLayoutBinding
    };

    VkDescriptorSetLayoutCreateInfo descriptorSetMemLayoutCreationInfo{};
    descriptorSetMemLayoutCreationInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetMemLayoutCreationInfo.bindingCount = bindings.size();
    descriptorSetMemLayoutCreationInfo.pBindings = bindings.data();

    if(vkCreateDescriptorSetLayout(logiDevice, &descriptorSetMemLayoutCreationInfo, nullptr, &mainDescriptorSetLayout) != VK_SUCCESS){
        throw std::runtime_error("failed to create main descriptor set layout. (MVP Mat and texture)");
    }

}

void HelloTriangleApplication::createCoherentUniformBuffers(){
    VkDeviceSize mvpMatSize = sizeof(ModelViewProjection);

    mvpMatUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    mvpMatUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
    mvpMatUniformBuffersMapHandles.resize(MAX_FRAMES_IN_FLIGHT);

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
        createAndBindDeviceBuffer(
            mvpMatSize,
            VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
            mvpMatUniformBuffers[i],
            mvpMatUniformBuffersMemory[i]
        );

        vkMapMemory(logiDevice, mvpMatUniformBuffersMemory[i], 0, mvpMatSize, 0, &mvpMatUniformBuffersMapHandles[i]);
    }
}

void HelloTriangleApplication::createDescriptorPool(){
    VkDescriptorPoolSize poolSizeMVP{};
    poolSizeMVP.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizeMVP.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolSize poolSizeMainTex{};
    poolSizeMainTex.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizeMainTex.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    std::array<VkDescriptorPoolSize, 2> poolSizes = {
        poolSizeMVP, poolSizeMainTex
    };
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    /* Maximum allocations expected by the program. Allows for optimization */
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    if(vkCreateDescriptorPool(logiDevice, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS){
        throw std::runtime_error("failed to create descriptor pool");
    }
}

void HelloTriangleApplication::createDescriptorSets(){
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, mainDescriptorSetLayout);
    VkDescriptorSetAllocateInfo descriptorSetAllocationInfo{};
    
    descriptorSetAllocationInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptorSetAllocationInfo.descriptorPool = descriptorPool;
    descriptorSetAllocationInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    descriptorSetAllocationInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    if(vkAllocateDescriptorSets(logiDevice, &descriptorSetAllocationInfo, descriptorSets.data()) != VK_SUCCESS ){
        throw std::runtime_error("failed to allocate descriptor sets");
    }

    for(size_t i = 0; i < descriptorSets.size(); i++){
        VkDescriptorBufferInfo bufferInfo;
        bufferInfo.buffer = mvpMatUniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(ModelViewProjection);

        VkDescriptorImageInfo mainTexInfo{};
        mainTexInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        mainTexInfo.imageView = textureImageView;
        mainTexInfo.sampler = textureSampler;
        

        VkWriteDescriptorSet writeMvpMatOp{};
        writeMvpMatOp.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeMvpMatOp.dstSet = descriptorSets[i];
        writeMvpMatOp.dstBinding = 0;
        writeMvpMatOp.dstArrayElement = 0;
        writeMvpMatOp.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        writeMvpMatOp.descriptorCount = 1;
        
        writeMvpMatOp.pBufferInfo = &bufferInfo;
        /*-- These next two are only used for other type of descriptors (e.g. texture samplers) --*/
        writeMvpMatOp.pImageInfo = nullptr;
        writeMvpMatOp.pTexelBufferView = nullptr;

        VkWriteDescriptorSet writeMainTexOp{};
        writeMainTexOp.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writeMainTexOp.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writeMainTexOp.dstBinding = 1;
        writeMainTexOp.dstArrayElement = 0;
        writeMainTexOp.descriptorCount = 1;
        writeMainTexOp.dstSet = descriptorSets[i];
        writeMainTexOp.pImageInfo = &mainTexInfo;

        std::array<VkWriteDescriptorSet, 2> writeDescriptorOps = {
            writeMvpMatOp, writeMainTexOp
        };

        vkUpdateDescriptorSets(logiDevice, static_cast<uint32_t>(writeDescriptorOps.size()), writeDescriptorOps.data(), 0, nullptr);
    }
}

void HelloTriangleApplication::createAndBindDeviceImage(int width, 
                                                        int height, 
                                                        VkSampleCountFlagBits samples,
                                                        VkImage& imageDescriptor, 
                                                        VkDeviceMemory& imageMemory, 
                                                        VkFormat format,
                                                        VkImageTiling tiling, 
                                                        VkImageUsageFlags usage, 
                                                        VkMemoryPropertyFlags memProperties, 
                                                        int mipmapLevels){

    VkImageCreateInfo imageCreationInfo{};
    imageCreationInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    /*
    VK_IMAGE_TYPE_1D : Basically a buffer of pixels (e.g. gradients)
    VK_IMAGE_TYPE_2D : Typical texture with x y coordinates 
    VK_IMAGE_TYPE_3D : Voxel textures / Cubemaps  
    */
    imageCreationInfo.imageType = VK_IMAGE_TYPE_2D;
    imageCreationInfo.extent.width = width;
    imageCreationInfo.extent.height = height;
    imageCreationInfo.extent.depth = 1; // For cubemaps
    imageCreationInfo.mipLevels = std::max(1, mipmapLevels);
    imageCreationInfo.arrayLayers = 1;
    imageCreationInfo.format = format;
    /*
    VK_IMAGE_TILING_OPTIMAL : Lets the underlying driver decide to improve memory access. 
    VK_IMAGE_TILING_LINEAR  : texels are ordered as if a matrix, allows for precise texel access.

    NB: This is for the LOW LEVEL MEMORY only, this will not affect UV access as it is done through the sampler.
    Think of it this way, LINEAR allows access to the raw memory same as a C pointer to an array, so adding to the pointer 
    the size of the element would give you the next element in the array.
    OPTIMAL chooses to forfeit this method of access to optimize access times, we almost never need to access texture memory
    in a linear way anyway but rather "randomly"(from a locality perspective).
    */
    imageCreationInfo.tiling = tiling;
    imageCreationInfo.usage  = usage;
    imageCreationInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageCreationInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; /* Only used by graphics queue */
    imageCreationInfo.samples = samples;
    imageCreationInfo.flags = 0;

    if(vkCreateImage(logiDevice, &imageCreationInfo, nullptr, &imageDescriptor) != VK_SUCCESS){
        throw std::runtime_error("failed to create texture on the device");
    }

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(logiDevice, imageDescriptor, &memReq);

    VkMemoryAllocateInfo textureAllocationInfo{};
    textureAllocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    textureAllocationInfo.allocationSize = memReq.size;
    textureAllocationInfo.memoryTypeIndex = getFirstUsableMemoryType(
        memReq.memoryTypeBits, 
        memProperties
    );

    if (vkAllocateMemory(logiDevice, &textureAllocationInfo, nullptr, &imageMemory) != VK_SUCCESS){
        throw std::runtime_error("could not allocate memory for the texture");
    }

    if(vkBindImageMemory(logiDevice, imageDescriptor, imageMemory, 0) != VK_SUCCESS){
        throw std::runtime_error("failed to bind the image to the handle");
    }
}


void HelloTriangleApplication::createTextureImage(){
    int texWidth, texHeight, texChannels;

    /*
    stbi_uc* points to the first pixel in an array of pixels of the image. Each pixels occupies 4 bytes.
    Order is row by row, so if an image is 100 wide and 200 high we will have:
    [pixels 0...99]
    [row 2] 
    [row 199] 
    */
    #ifdef HELIUM_LOAD_MODEL
    stbi_uc* firstPixelPointer = loadImage(TEX_PATH.c_str(), &texWidth, &texHeight, &texChannels);
    #else
    stbi_uc* firstPixelPointer = loadImage("/Users/kambo/Helium/GameDev/Projects/CGSamples/Vulkan/textures/tex.jpg", &texWidth, &texHeight, &texChannels);
    #endif
    textureMipmaps = static_cast<uint32_t>(std::floor(
        std::log2(std::max(texWidth, texHeight)))
    ) + 1; // +1 because of level 0
    VkDeviceSize imageSize = texWidth * texHeight * 4;
    if (!firstPixelPointer) {
        throw std::runtime_error("failed to load texture");
    }

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;

    createAndBindDeviceBuffer(imageSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    stagingBuffer,
    stagingMemory);

    void *data;
    vkMapMemory(logiDevice, stagingMemory, 0, imageSize, 0, &data);
    memcpy(data, firstPixelPointer, static_cast<size_t>(imageSize));
    vkUnmapMemory(logiDevice, stagingMemory);

    stbi_image_free(firstPixelPointer);

    VkFormat selectedFormat = VK_FORMAT_R8G8B8A8_SRGB; // 8b * 4 = 32bits = 4 bytes per pixel from before
    createAndBindDeviceImage(
        texWidth,
        texHeight, 
        VK_SAMPLE_COUNT_1_BIT,
        textureImageHandle, 
        textureImageDeviceMemory, 
        selectedFormat, 
        VK_IMAGE_TILING_OPTIMAL, 
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        textureMipmaps
    );

    std::cout << "creating first image layout conversion" << std::endl;
    convertImageLayout(textureImageHandle, textureMipmaps, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    bufferCopyToImage(
        stagingBuffer,
        textureImageHandle, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight)
    );
    std::cout << "creating second image layout conversion" << std::endl;
    generatateImageMipMaps(
        textureImageHandle, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, textureMipmaps
    );

    vkDestroyBuffer(logiDevice, stagingBuffer, nullptr);
    vkFreeMemory(logiDevice, stagingMemory, nullptr);
}

void HelloTriangleApplication::createTextureImageView(){
    textureImageView = createViewFor2DImage(
        textureImageHandle, 
        textureMipmaps,
        VK_FORMAT_R8G8B8A8_SRGB,
        VK_IMAGE_ASPECT_COLOR_BIT
    );
}

void HelloTriangleApplication::createTextureSampler(){
    VkSamplerCreateInfo samplerCreationInfo{};
    samplerCreationInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    // Filtering for magnified texels (a texel overlaps multiple fragments)
    samplerCreationInfo.magFilter = VK_FILTER_LINEAR;
    // Filtering for minified texels (multiple texels overlap in a fragment)
    samplerCreationInfo.minFilter = VK_FILTER_LINEAR;
    samplerCreationInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    samplerCreationInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    samplerCreationInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    samplerCreationInfo.anisotropyEnable = VK_TRUE;
    samplerCreationInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerCreationInfo.unnormalizedCoordinates = VK_FALSE; // If trues you use the number of pixels as coordinates. 
    samplerCreationInfo.compareEnable = VK_FALSE;
    samplerCreationInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerCreationInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerCreationInfo.mipLodBias = 0.0f;
    samplerCreationInfo.minLod = 0.0f;
    samplerCreationInfo.maxLod = static_cast<float>(textureMipmaps);
    
    VkPhysicalDeviceProperties physicalDeviceProperties{};
    vkGetPhysicalDeviceProperties(physGraphicDevice, &physicalDeviceProperties);
    // This will use maximum number of anisotropic samples, ensuring best quality
    // this affects performance, so lowering this improves perf.
    samplerCreationInfo.maxAnisotropy = physicalDeviceProperties.limits.maxSamplerAnisotropy;

    if(vkCreateSampler(logiDevice, &samplerCreationInfo, nullptr, &textureSampler) != VK_SUCCESS){
        throw std::runtime_error("failed to create sampler for main texture");
    }
    std::cout << "texture sampler done: " << textureSampler << std::endl;

}

void HelloTriangleApplication::convertImageLayout(VkImage srcImage, int mipmapLevels, VkFormat format, VkImageLayout srcLayout, VkImageLayout dstLayout){
    VkCommandBuffer oneTimeBuffer = beginOneTimeCommands();

    /* Memory barriers not only act as synchronizers in the pipeline, but
        also act as checkpoints for memory ownership transfers (e.g. from graphics queue to presentation queue)
        as well as layout conversions. */
    VkImageMemoryBarrier layoutConversionBarrier{};
    layoutConversionBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    layoutConversionBarrier.oldLayout = srcLayout;
    layoutConversionBarrier.newLayout = dstLayout;
    layoutConversionBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    layoutConversionBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

    layoutConversionBarrier.image = srcImage;
    if(dstLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
        layoutConversionBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if(hasStencilComponent(format)){
            layoutConversionBarrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }else{
        layoutConversionBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    layoutConversionBarrier.subresourceRange.baseMipLevel = 0;
    layoutConversionBarrier.subresourceRange.levelCount = std::max(1, mipmapLevels);
    layoutConversionBarrier.subresourceRange.baseArrayLayer = 0;
    layoutConversionBarrier.subresourceRange.layerCount = 1;

    layoutConversionBarrier.srcAccessMask = 0;
    layoutConversionBarrier.dstAccessMask = 0;


    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        layoutConversionBarrier.srcAccessMask = 0;
        layoutConversionBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (srcLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && dstLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        layoutConversionBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        layoutConversionBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if(srcLayout == VK_IMAGE_LAYOUT_UNDEFINED && dstLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL){
        layoutConversionBarrier.srcAccessMask = 0;
        layoutConversionBarrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else{
        throw std::invalid_argument("unsupported layout transition!");
    }

    vkCmdPipelineBarrier(
        oneTimeBuffer, sourceStage,
        destinationStage,
        0, 0, nullptr, 0, nullptr, 1 , &layoutConversionBarrier
    );

    endAndSubmitOneTimeCommands(oneTimeBuffer);
}


void HelloTriangleApplication::createDepthPassResources(){
    VkFormat format = findFirstSupportedDepthFormatFromDefaults();

    createAndBindDeviceImage(
        selectedSwapChainWindowSize.width,
        selectedSwapChainWindowSize.height,
        maxMsaaSupported,
        depthPassImage,
        depthPassMemory,
        format,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        1
    );

    depthPassImageView = createViewFor2DImage(
        depthPassImage,
        1,
        format,
        VK_IMAGE_ASPECT_DEPTH_BIT
    );

    /*--- Unnecessary steps technically because they are done in the render pass as well. Here for completeness ---*/
    convertImageLayout(
        depthPassImage,
        1,
        format,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
    );
    
}

VkFormat HelloTriangleApplication::findFirstSupportedDepthFormatFromDefaults(){
    std::vector<VkFormat> formats = {
        VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT
    };
    VkImageTiling desiredTiling = VK_IMAGE_TILING_OPTIMAL;
    VkFormatFeatureFlags desiredFeatures = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;

    return findFirstSupportedDepthFormat(
        formats, 
        desiredTiling,
        desiredFeatures
    );
}


VkFormat HelloTriangleApplication::findFirstSupportedDepthFormat(const std::vector<VkFormat>& availableFormats, VkImageTiling desiredTiling, VkFormatFeatureFlags requiredFeatures){
    for (VkFormat f: availableFormats){
        VkFormatProperties formatProperties;
        vkGetPhysicalDeviceFormatProperties(
            physGraphicDevice,
            f,
            &formatProperties
        );
        if(
            desiredTiling == VK_IMAGE_TILING_LINEAR && 
            (formatProperties.linearTilingFeatures & requiredFeatures) == requiredFeatures
        ){
            return f;
        }
        if(
            desiredTiling == VK_IMAGE_TILING_OPTIMAL &&
            (formatProperties.optimalTilingFeatures & requiredFeatures) == requiredFeatures
        ){
            return f;
        }
    }
    throw std::runtime_error("did not find format with required tiling");
}


#endif 

void HelloTriangleApplication::createMsaaColorResources(){
    VkFormat colorFormat = selectedSwapChainFormat;

    createAndBindDeviceImage(
        selectedSwapChainWindowSize.width,
        selectedSwapChainWindowSize.height,
        maxMsaaSupported,
        msaaColorImage,
        msaaColorMemory,
        colorFormat,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, //only used by gpu for sampling from this color framebuffer to swapchain fb
        1
    );

    msaaColorView = createViewFor2DImage(
        msaaColorImage,
        1,
        colorFormat,
        VK_IMAGE_ASPECT_COLOR_BIT
    );
}

void HelloTriangleApplication::bufferCopyToImage(VkBuffer srcBuffer, VkImage dstImage, uint32_t w, uint32_t h){
    VkCommandBuffer oneTimeBuffer = beginOneTimeCommands();
    
    VkBufferImageCopy imageCopyOp{};
    imageCopyOp.bufferOffset = 0;
    imageCopyOp.bufferRowLength = 0;
    imageCopyOp.bufferImageHeight = 0;
    
    imageCopyOp.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    imageCopyOp.imageSubresource.mipLevel = 0;
    imageCopyOp.imageSubresource.baseArrayLayer = 0;
    imageCopyOp.imageSubresource.layerCount = 1;
    imageCopyOp.imageOffset = {0,0,0};
    imageCopyOp.imageExtent= {
        w,
        h,
        1
    };

    vkCmdCopyBufferToImage(
        oneTimeBuffer, srcBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageCopyOp
    );

    endAndSubmitOneTimeCommands(oneTimeBuffer);
}

VkCommandBuffer HelloTriangleApplication::beginOneTimeCommands(){
    VkCommandBufferAllocateInfo tempBufferCreationInfo{};
    tempBufferCreationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    tempBufferCreationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    tempBufferCreationInfo.commandPool = commandPool;
    tempBufferCreationInfo.commandBufferCount = 1;

    VkCommandBuffer tempBuffer;
    vkAllocateCommandBuffers(logiDevice, &tempBufferCreationInfo, &tempBuffer);

    /*----- Record the buffer -----*/
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(tempBuffer, &beginInfo);

    return tempBuffer;
}

void HelloTriangleApplication::endAndSubmitOneTimeCommands(VkCommandBuffer buffer){
    vkEndCommandBuffer(buffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer;

    vkQueueSubmit(graphicsCommandQueue, 1, &submitInfo, VK_NULL_HANDLE);
    // Wait for queue to be empty before continuing. Makes sure the full buffer is copied.
    vkQueueWaitIdle(graphicsCommandQueue);

    vkFreeCommandBuffers(logiDevice, commandPool, 1, &buffer);
}


// Copies from src to dst a {size} amount of bytes. It uses the graphics command queue and waits for it to be idle.
// Not the best perf. wise.
void HelloTriangleApplication::bufferCopy(VkBuffer src, VkBuffer dst, VkDeviceSize size){
    VkCommandBuffer oneTimeCommandBuffer = beginOneTimeCommands();
    /* Describes the region to copy by it's start index on both buffers and the amount of bytes */
    VkBufferCopy copyOpDesc{};
    copyOpDesc.srcOffset = 0;
    copyOpDesc.dstOffset = 0;
    copyOpDesc.size = size;

    vkCmdCopyBuffer(oneTimeCommandBuffer, src, dst, 1, &copyOpDesc);

    endAndSubmitOneTimeCommands(oneTimeCommandBuffer);

}

uint32_t HelloTriangleApplication::getFirstUsableMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags requiredPropertyFlags){
    VkPhysicalDeviceMemoryProperties memProps;
    vkGetPhysicalDeviceMemoryProperties(physGraphicDevice, &memProps);

    for (uint32_t i = 0 ; i < memProps.memoryTypeCount; i++){
        if (typeFilter & (1 << i) && (memProps.memoryTypes[i].propertyFlags & requiredPropertyFlags) == requiredPropertyFlags){
            return i;
        }
    }

    throw std::runtime_error("cannot adapt memory to underlying hardware");
}

void HelloTriangleApplication::createFramebuffers(){
    swapchainFramebuffers.resize(swapChainImageViews.size());

    for (size_t i =0 ; i < swapChainImageViews.size(); i++){
        std::array<VkImageView,3> iv = {
            // Order cannot be random
            // 1. Present view
            // 2. Depth View 
            // 3. Color view (for MSAA)
            msaaColorView,
            depthPassImageView,
            swapChainImageViews[i] 
        };

        VkFramebufferCreateInfo framebufferCreationInfo{};
        framebufferCreationInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreationInfo.height = selectedSwapChainWindowSize.height;
        framebufferCreationInfo.width = selectedSwapChainWindowSize.width;
        framebufferCreationInfo.attachmentCount = static_cast<uint32_t>(iv.size());
        framebufferCreationInfo.pAttachments = iv.data();
        framebufferCreationInfo.renderPass = renderPass;
        framebufferCreationInfo.layers = 1;
        VkResult res = vkCreateFramebuffer(logiDevice, &framebufferCreationInfo, nullptr, &swapchainFramebuffers[i]);
        if(res != VK_SUCCESS){
            throw std::runtime_error("failed to create framebuffer");
        }
    }
}

void HelloTriangleApplication::createCommandPool(){
    QueueFamilyIndices qfi = findRequiredQueueFamily(physGraphicDevice);

    // Why are command pools a thing if command buffers which create commands for the queues exist?
    // Mostly the following reasons
    // - Abstracting from the underlying implementation (we don't need to know what queues are there, but the pools)
    // - Memory optimization (when freeing and allocating new buffers for the same pool Vulkan might use the same memory)
    // - Lifetime management (when pool dies buffers die, no risk of having stray buffers still attached to the queues)
    // So essentially you can X buffers going into the same pool for the same queue, when the pool dies all of them get destroyed.
    // This last line might be wrong, but it makes sense since you allocate a command buffer FROM the pool.
    VkCommandPoolCreateInfo poolCreationInfo{};
    poolCreationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    /*
    Define basically the lifetime of the command buffers in this pool. Which in turn defines how the memory is allocated for them.
    CREATE_TRANSIENT_BIT             : The command buffers allocated from this pool will be short-lived
    CREATE_RESET_COMMAND_BUFFER_BIT  : Allows manual reset of each command buffer from this pool. Needs to be set in order to call vkResetCommandBuffer. 
    CREATE_PROTECTED_BIT             : Cannot be reset. Basically static command buffers that are immutable.

    Optimization is in theory, because if RESET_COMMAND_BUFFER_BIT you can then dispatch commands at each frame (thus the reset)
    and so the calls to fill the buffer might be grossly unoptimized.
    */
    poolCreationInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    // All command buffers allocated from this pool have to be submitted to the following queue.
    // So this basically means this pool will always dispatch graphcis related commands. 
    // All commands in cbuffers have to be submitted to a device queue.
    // Also, given the creation info 1 pool<=>1 device queue
    poolCreationInfo.queueFamilyIndex = qfi.graphicsFamilyIndex.value();
    if(vkCreateCommandPool(logiDevice, &poolCreationInfo, nullptr, &commandPool) != VK_SUCCESS){
        throw std::runtime_error("failed to create graphics command pool");
    }
}

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

void HelloTriangleApplication::createCommandBuffers(){
    graphicsCBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo graphicsCBufferAllocationInfo{};
    graphicsCBufferAllocationInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    graphicsCBufferAllocationInfo.commandPool = commandPool;
    graphicsCBufferAllocationInfo.commandBufferCount = (uint32_t) graphicsCBuffers.size();
    /*
    PRIMARY     : Directly submitted to queues. Can execute secondary command buffers.
    SECONDARY   : Cannot be directly submitted to queues. (Useful for example for redoing the same operation without having to rebuild it.)
                    e.g. (replay secondary buffer -> do other stuff -> replay -> replay -> replay etc... without changing the secondary buffer)
    */
    graphicsCBufferAllocationInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

    VkResult allocationResult = vkAllocateCommandBuffers(logiDevice, &graphicsCBufferAllocationInfo, graphicsCBuffers.data()) ;
    if (allocationResult!= VK_SUCCESS){
        throw std::runtime_error("failed to allocate graphics command buffer");
    }
}