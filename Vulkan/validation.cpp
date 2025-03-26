#include "main.h"


bool HelloTriangleApplication::checkValidationLayerSupport(){
    uint32_t count;
    vkEnumerateInstanceLayerProperties(&count, nullptr);

    std::vector<VkLayerProperties> layers(count);
    vkEnumerateInstanceLayerProperties(&count, layers.data());
    #ifdef HELIUM_PRINT_LAYERS
    std::cout << "available instance layers:" << std::endl;
    for (const auto& l: layers){
        std::cout << l.layerName << std::endl;
    }
    #endif
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