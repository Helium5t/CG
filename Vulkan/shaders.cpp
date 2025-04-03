#include "main.h"

VkShaderModule HelloTriangleApplication::createShaderModule(const std::vector<char> binary){
    VkShaderModule shaderModule;

    VkShaderModuleCreateInfo shaderCreationInfo{};
    shaderCreationInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    // reinterpret_cast simply tells compiler to treat pointer as the target pointer. So not very safe
    shaderCreationInfo.pCode = reinterpret_cast<const uint32_t*>(binary.data()); 
    // shaderCreationInfo.flags is  not set
    shaderCreationInfo.codeSize = binary.size();

    if (vkCreateShaderModule(logiDevice, &shaderCreationInfo, nullptr, &shaderModule) != VK_SUCCESS){
        throw std::runtime_error("failed to create shader");
    }
    return shaderModule;
}