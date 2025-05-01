#define STB_IMAGE_IMPLEMENTATION
#include "main.h"

stbi_uc* HelloTriangleApplication::loadImage(const char* path, int* width, int* height, int* channels){
    int a,b,c;
    stbi_uc* pixels = stbi_load(path, width, height, channels, STBI_rgb_alpha);
    return pixels;
}


/*
Creates a view for a 2D image with no mip map levels and no layers.
View will be generated with no swizzle. 
*/
VkImageView HelloTriangleApplication::createViewFor2DImage(VkImage image, VkFormat format, VkImageAspectFlags imageAspect){
    VkImageView view;

    VkImageViewCreateInfo imageViewCreationInfo{};
    imageViewCreationInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreationInfo.format = format;
    imageViewCreationInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreationInfo.image = image;
    imageViewCreationInfo.subresourceRange.aspectMask = imageAspect;
    imageViewCreationInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreationInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreationInfo.subresourceRange.levelCount = 1;
    // Multiple layers can be useful for steroscopic apps (VR) and have each eye map to a layer
    imageViewCreationInfo.subresourceRange.layerCount = 1;
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
    imageViewCreationInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreationInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreationInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    imageViewCreationInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    if(vkCreateImageView(logiDevice, &imageViewCreationInfo, nullptr, &view) != VK_SUCCESS){
        throw std::runtime_error("failed to create view for image");
    }
    return view;
}