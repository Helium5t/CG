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
    iInfo.enabledExtensionCount ++;
    requiredExtNames.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    // iInfo.enabledExtensionCount ++;
    // requiredExtNames.emplace_back("VK_KHR_portability_subset");
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
            &swapChainImageViews[i]
        );
        if (viewCreationResult != VK_SUCCESS){
            throw std::runtime_error(strcat("Failed to create image view:", VkResultToString(viewCreationResult)));
        }
    }
}

void HelloTriangleApplication::createRenderPass(){
    VkAttachmentDescription attachmentDescription{}; 
    attachmentDescription.format = selectedSwapChainFormat; // Image format i.e. bits per channel and linear/gamma etc...
    attachmentDescription.samples = VK_SAMPLE_COUNT_1_BIT; // Only one sample
    /*
    Defines what to do before the render pass
    LOAD : Preserve what is already there
    CLEAR : Clear all memory
    DONT_CARE : No need to preserve previous data or define behaviour, let underlying implementation do whatever it wants.
    NONE : The image is not accessed at all during the render pass, do nothing.
    */
    attachmentDescription.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clears the screen right before writing to frame buffer
    /*
    Defines what to do after the render pass, has same values as load except CLEAR*/
    attachmentDescription.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // stores the rendered frame into the frame buffer.
    // Previous ops were for color and depth buffers, all stencil buffers follow instead these two ops
    attachmentDescription.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; 
    attachmentDescription.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
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
    attachmentDescription.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // pre-pass
    attachmentDescription.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; //post-pass

    // Used to set the binding for the render pass
    VkAttachmentReference attachmentRef{};
    attachmentRef.attachment = 0; // reference to first attachment (we have only one atm)
    attachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Use as color attachment (output)

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
    subpassDesc.pColorAttachments = &attachmentRef;

    // Add dependency to make sure pipeline waits for the image to be writeable
    VkSubpassDependency dependency{};
    // Special bit, specified the pre-pipeline implicit pass or the post-pipeline implicit pass
    // respectively if it's in srcSubpass or dstSubpass.
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL; 
    dependency.dstSubpass = 0;

    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;

    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


    VkRenderPassCreateInfo renderPassCreationInfo{};
    renderPassCreationInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreationInfo.attachmentCount = 1;
    renderPassCreationInfo.pAttachments = &attachmentDescription;
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
    std::vector<char> vShaderBinary = readFile("/Users/kambo/Helium/GameDev/Projects/CGSamples/Vulkan/shaders/helloTriangle_v.spv");
    #else

    VkVertexInputBindingDescription bindingDescription = Vert::getBindingDescription();
    std::array<VkVertexInputAttributeDescription, 2> attributeDescription = Vert::getAttributeDescription();
    

    std::vector<char> vShaderBinary = readFile("/Users/kambo/Helium/GameDev/Projects/CGSamples/Vulkan/shaders/dynamicVertex_v.spv");
    #endif 
    std::vector<char> fShaderBinary = readFile("/Users/kambo/Helium/GameDev/Projects/CGSamples/Vulkan/shaders/helloTriangle_f.spv");

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
    rasterizationStageCreationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
    // Disabled so 0/min values
    // Enable running fragment shader once for each sample rather than each pixel => multiple times per pixel based on amount of overlapping geometry
    multisamplingStageCreationInfo.sampleShadingEnable = VK_FALSE; 
    multisamplingStageCreationInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT; // num of samples
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

    VkPipelineDepthStencilStateCreateInfo* depthStencilStageCreationInfo = nullptr;// disabled for now, nullptr is passed

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
    pipelineLayoutCreationInfo.setLayoutCount = 0;
    pipelineLayoutCreationInfo.pSetLayouts = nullptr;
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
    pipelineCreationInfo.pDepthStencilState = depthStencilStageCreationInfo; // actually null
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

void HelloTriangleApplication::createDeviceVertexBuffer(){
    #ifndef HELIUM_VERTEX_BUFFERS
    return;
    #else
    VkBufferCreateInfo bufferCreationInfo{};
    bufferCreationInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferCreationInfo.size = sizeof(vertices[0]) * vertices.size();
    bufferCreationInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    /*
    Sharing mode is for the queue, not for the shaders. Only the graphics queue 
    will use the buffer so exclusive.
    */
   bufferCreationInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
   
   if (vkCreateBuffer(logiDevice, &bufferCreationInfo, nullptr, &vertexBuffer) != VK_SUCCESS){
       throw std::runtime_error("failed to create the vertex buffer object on the device");
    }
    
    /*----- Allocate memory -----*/
    VkMemoryRequirements memReqs;
    vkGetBufferMemoryRequirements(logiDevice, vertexBuffer, &memReqs);
    
    VkMemoryAllocateInfo allocationInfo{};
    allocationInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocationInfo.allocationSize = memReqs.size;
    allocationInfo.memoryTypeIndex = getFirstUsableMemoryType(
        memReqs.memoryTypeBits,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        
    if(vkAllocateMemory(logiDevice, &allocationInfo, nullptr, &vertexBufferMemory) != VK_SUCCESS){
        throw std::runtime_error("failed to allocate buffer device memory for vertex buffer object");
    }
    
    /*----- Bind allocated memory to vertex buffer object -----*/
    vkBindBufferMemory(logiDevice, vertexBuffer, vertexBufferMemory, 0);
    
    
    /*----- Write to device buffer -----*/
    void* bufferData;
    // Get virtual address pointer to the device buffer
    vkMapMemory(logiDevice, vertexBufferMemory, 0, bufferCreationInfo.size, 0, &bufferData);
    // Write to the memory via memcpy, copying the vertex buffer content into the device memory
    memcpy(bufferData, vertices.data(), (size_t) bufferCreationInfo.size);
    vkUnmapMemory(logiDevice, vertexBufferMemory);

    #endif
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
        VkImageView iv[] = {
            swapChainImageViews[i]
        };

        VkFramebufferCreateInfo framebufferCreationInfo{};
        framebufferCreationInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreationInfo.renderPass = renderPass;
        framebufferCreationInfo.layers = 1;
        framebufferCreationInfo.attachmentCount = 1;
        framebufferCreationInfo.pAttachments = iv;
        framebufferCreationInfo.height = selectedSwapChainWindowSize.height;
        framebufferCreationInfo.width = selectedSwapChainWindowSize.width;
        bool isHeRight = iv[0] == VK_NULL_HANDLE;
        std::cout <<  "is he right?" << isHeRight << std::endl;
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

    vkCmdDraw(buffer, static_cast<uint32_t>(vertices.size()), 1, 0, 0);
    #else
    vkCmdDraw(buffer, 3, 1, 0, 0);
    #endif

    vkCmdEndRenderPass(buffer);

    if (vkEndCommandBuffer(buffer) != VK_SUCCESS){
        throw std::runtime_error("failed to record the graphics command buffer");
    }
}