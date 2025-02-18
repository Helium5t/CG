// #include <vulkan/vulkan.h> // Provides funcs, structs and enums for vulkan
// replaces line 1 


#include <iostream>
#include <stdexcept>
#include <cstdlib> // Used for the two success statuses EXIT_SUCCESS and EXIT_FAILURE 
#include <map>
#include <set>
#include "heliumutils.h"
#include "heliumdebug.h"
#include <optional>

class HelloTriangleApplication {
public:
    void run() {
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

private:
    // Const params
    const uint32_t WIDTH = 800;
    const uint32_t HEIGHT = 600;

    
    const std::vector<const char*> validationLayerNames = {
    "VK_LAYER_KHRONOS_validation" // Standard bundle of validation layers included in LunarG SDK
    };

    #ifdef NDEBUG
    const bool validationLayerEnabled = false;
    #else
    const uint32_t validationLayerEnabled = true;
    #endif

    // Objects
    GLFWwindow* window;
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugCallbackHandler;
    VkSurfaceKHR renderSurface; // Where each render will be presented

    
    // Automatically destroyed by vulkan so no need to destroy it in cleanup
    VkPhysicalDevice physGraphicDevice = VK_NULL_HANDLE;  
    // The logical device (code interface for the physical device)
    VkDevice logiDevice; 
    VkQueue graphicsCommandQueue;
    VkQueue presentCommandQueue;

    void initWindow() {
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

    bool checkValidationLayerSupport(){
        uint32_t count;
        vkEnumerateInstanceLayerProperties(&count, nullptr);

        std::vector<VkLayerProperties> layers(count);
        vkEnumerateInstanceLayerProperties(&count, layers.data());
        for (const char* l : validationLayerNames){
            bool found = false;
            for (VkLayerProperties lp : layers){
                if(strcmp(l, lp.layerName) == 0){
                    found = true;
                    break;
                }
            }
            if (!found){
                return false;
            }
        }
        return true;
    }

    std::vector<const char*> getRequiredExtensions(){
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

    void createInstance(){
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


    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        setupRenderSurface();
        setPhysicalDevice();
        createLogicalDevice();
    }

    void setupRenderSurface(){
        VkResult createRenderSurfaceResult = glfwCreateWindowSurface(instance, window, nullptr, &renderSurface);
        if (createRenderSurfaceResult != VK_SUCCESS){
            throw std::runtime_error(strcat("Failed to create render surface: ", VkResultToString(createRenderSurfaceResult)));
        }
    }

    void createLogicalDevice(){
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

        logicalDeviceCreationInfo.enabledExtensionCount = 0; // No extensions needed atm.

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

    // Builds the vulkan representation of the used GPU
    void setPhysicalDevice(){
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
        if (deviceCount == 0) {
            throw std::runtime_error("No Vulkan Supported GPUs found");
        }
        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());
        std::multimap<int, VkPhysicalDevice> possibleDevices;
        for( const auto& device : devices){
            int supportScore = rateDevice(device);
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

    // Contains the index of the queue family we need. The index refers to the position in the array returned by vkGetPhysicalDeviceQueueFamilyProperties.
    struct QueueFamilyIndices{
        // Queue family for graphics commands
        std::optional<uint32_t> graphicsFamilyIndex; // 0 is a valid family index (each index represents a queue family that supports certain commands) so we need a way to discern between null and 0.
        // Queue family for presentation to surface
        std::optional<uint32_t> presentationFamilyIndex; 

        bool has_values(){
            return graphicsFamilyIndex.has_value() && presentationFamilyIndex.has_value();
        }
    };

    QueueFamilyIndices findRequiredQueueFamily(VkPhysicalDevice vkpd){
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

    bool rateDevice(VkPhysicalDevice vkpd){
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
        if (familyIndex.has_values()){
            return true;
        }
        return false;
    }

    void fillCreateInfoForDebugHandler(VkDebugUtilsMessengerCreateInfoEXT& toBeFilled){
        std::cout<< "create info for debug handler" << std::endl;
        toBeFilled.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        toBeFilled.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        toBeFilled.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT | VK_TOOL_PURPOSE_VALIDATION_BIT_EXT;
        toBeFilled.pfnUserCallback = parseDebugCallbackInstance;
        // This will be passed back to the debug handler when emitting the callback, this way you can access some data you want to emit into the debug callback 
        toBeFilled.pUserData = nullptr;
        // Added because I was getting a warning
        toBeFilled.flags = 0;
    }

    void setupDebugMessenger(){
        if(!validationLayerEnabled) return;
        VkDebugUtilsMessengerCreateInfoEXT mcInfo;
        fillCreateInfoForDebugHandler(mcInfo);
        mcInfo.pfnUserCallback = parseDebugCallbackLoop; /* To check if this actually works later on*/
        if (CreateDebugMessengerExtension(instance /* <- Debug messengers are specific to instances and layers, so we need this */, &mcInfo, nullptr, &debugCallbackHandler) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger");
        }
    }

    void mainLoop() {
        while( !glfwWindowShouldClose(window)){
            glfwPollEvents();
        }
    }

    void cleanup() {
        if(validationLayerEnabled){
            DestroyDebugMessengerExtension(instance, debugCallbackHandler, nullptr); // Ideally this should be caught by the debug messenger when destroy is not called, and yet it doesn't happen
        }
        // In order : Device generating renders -> render surface -> instance -> window -> glfw.
        vkDestroyDevice(logiDevice, nullptr);
        vkDestroySurfaceKHR(instance, renderSurface, nullptr);
        vkDestroyInstance(instance, nullptr/*Optional callback pointer*/);
        glfwDestroyWindow(window);
        glfwTerminate(); // Once this function is called, glfwInit(L#30) must be called again before using most GLFW functions. This deallocates everything GLFW related.
    }
};

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