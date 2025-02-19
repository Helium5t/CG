#include "main.h"


bool HelloTriangleApplication::checkValidationLayerSupport(){
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