#include "main.h"

VkVertexInputBindingDescription Vert::getBindingDescription(){
    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vert);
    /*
    VK_VERTEX_INPUT_RATE_VERTEX = 0, // Binding is constant for each vertex
    VK_VERTEX_INPUT_RATE_INSTANCE = 1, // Binding is constant for each instance (multiple vertices will read the same bound data)
    */
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    return bindingDescription;
}

/* We have three bindings:
 location 0 (inPosition)
 color 1 (inColor)
 texCoords 2 (uv) */
std::array<VkVertexInputAttributeDescription, 3> Vert::getAttributeDescription(){
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions{};
    // The binding for Vert is defined at index 0 so 0.
    attributeDescriptions[0].binding = 0; // Binding index  from where the attribute takes data from ( the .binding from the binding descriptor)
    attributeDescriptions[0].location = 0; // number in the layout of the shader (layout(location =....))
    // Lots of values, refer to https://registry.khronos.org/vulkan/specs/latest/man/html/VkFormat.html
    /*
    Most important ones:
        float: VK_FORMAT_R32_SFLOAT
        vec2: VK_FORMAT_R32G32_SFLOAT
        vec3: VK_FORMAT_R32G32B32_SFLOAT
        vec4: VK_FORMAT_R32G32B32A32_SFLOAT
    But basically it's as if you dedicate X bits to each color channel based on the type.
    a vector of integer (ivec2) would be VK_FORMAT_R32G32_SINT. 
        SINT = signed integer (int)
        UINT = uint
        SFLOAT = Signed float (both double and float)
    */
    attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT; // Type of data
    attributeDescriptions[0].offset = offsetof(Vert, pos); // Byte offset of the field in the struct

    attributeDescriptions[1].binding = 0; // Vert is still bound at 0
    attributeDescriptions[1].location = 1; // inColor is at 1
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT; // vec3 of floats
    attributeDescriptions[1].offset = offsetof(Vert, col);

    /*-----main tex-----*/
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vert, texCoords);
    return attributeDescriptions;
}
