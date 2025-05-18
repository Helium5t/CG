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
VkImageView HelloTriangleApplication::createViewFor2DImage(VkImage image, int mipmapLevels, VkFormat format, VkImageAspectFlags imageAspect){
    VkImageView view;

    VkImageViewCreateInfo imageViewCreationInfo{};
    imageViewCreationInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageViewCreationInfo.format = format;
    imageViewCreationInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    imageViewCreationInfo.image = image;
    imageViewCreationInfo.subresourceRange.aspectMask = imageAspect;
    imageViewCreationInfo.subresourceRange.baseArrayLayer = 0;
    imageViewCreationInfo.subresourceRange.baseMipLevel = 0;
    imageViewCreationInfo.subresourceRange.levelCount = std::max(1, mipmapLevels);
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

void HelloTriangleApplication::generatateImageMipMaps(VkImage image, VkFormat f, int32_t sourceWidth, int32_t sourceHeight, uint32_t levels){
    VkFormatProperties formatProps;
    vkGetPhysicalDeviceFormatProperties(
        physGraphicDevice, f, &formatProps
    );
    // TODO: check other format features, implement software mipmapping via stb_image_resize.
    if ((formatProps.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT) == 0){
        throw std::runtime_error("image format does not support linear filter sampling");
    }


    VkCommandBuffer cb = beginOneTimeCommands();

    VkImageMemoryBarrier mipmapBarrier = {};
    mipmapBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    mipmapBarrier.image = image;
    mipmapBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mipmapBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mipmapBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    mipmapBarrier.subresourceRange.baseArrayLayer = 0;
    mipmapBarrier.subresourceRange.layerCount = 1;
    mipmapBarrier.subresourceRange.levelCount = 1;

    for (uint32_t mip = 1; mip < levels; mip++){
        // Turn the previous level into a readonly source layer since it has already been generated
        mipmapBarrier.subresourceRange.baseMipLevel = mip-1;
        mipmapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        mipmapBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        mipmapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        mipmapBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

        vkCmdPipelineBarrier( /*Wait on source barrier*/
            cb,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,0,
            0, nullptr,
            0, nullptr,
            1, &mipmapBarrier
        );

        VkImageBlit mipBlitOp{};
        mipBlitOp.srcOffsets[0] = {0,0,0}; // Bounds of the blit source region. (size of the source image, 3D for image arrays)
        mipBlitOp.srcOffsets[1] = {sourceWidth,sourceHeight,1};
        mipBlitOp.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        mipBlitOp.srcSubresource.mipLevel = mip - 1;
        mipBlitOp.srcSubresource.baseArrayLayer = 0; // Where the copy starts from
        mipBlitOp.srcSubresource.layerCount = 1; // only previous mip
        mipBlitOp.dstOffsets[0] = {0,0,0};

        sourceWidth = std::max(1,sourceWidth >> 1);
        sourceHeight = std::max(1,sourceHeight >> 1);
        mipBlitOp.dstOffsets[1] = {
            sourceWidth,
            sourceHeight,
            1
        };
        mipBlitOp.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        mipBlitOp.dstSubresource.mipLevel = mip;
        mipBlitOp.dstSubresource.baseArrayLayer = 0;
        mipBlitOp.dstSubresource.layerCount = 1;

        vkCmdBlitImage(
            cb,
            image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
            image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &mipBlitOp,
            VK_FILTER_LINEAR
        );
        // make previous mip levels shader readonly now that we dont need it for mipping
        mipmapBarrier.subresourceRange.baseMipLevel = mip-1; 
        mipmapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        mipmapBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        mipmapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        mipmapBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,0,nullptr,0,nullptr, 1, &mipmapBarrier
        );
    }
    // turn last image to shader read only layout
    mipmapBarrier.subresourceRange.baseMipLevel = levels-1; 
    mipmapBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    mipmapBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    mipmapBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    mipmapBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        cb, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,0,nullptr,0,nullptr, 1, &mipmapBarrier
    );

    endAndSubmitOneTimeCommands(cb);
}