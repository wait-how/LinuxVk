#include "main.h"

VkImageView appvk::createImageView(VkImage im, VkFormat format, unsigned int mipLevels, VkImageAspectFlags aspectMask) {
    VkImageViewCreateInfo createInfo{};

    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image = im;
    createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format = format;
    
    VkComponentMapping map;
    map.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    map.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    map.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    map.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    createInfo.components = map;

    VkImageSubresourceRange range{};
    range.aspectMask = aspectMask;
    range.levelCount = mipLevels;
    range.layerCount = 1;

    createInfo.subresourceRange = range;

    VkImageView view = VK_NULL_HANDLE;
    if (vkCreateImageView(dev, &createInfo, nullptr, &view) != VK_SUCCESS) {
        throw std::runtime_error("cannot create image view!");
    }

    return view;
}

VkFormat appvk::findImageFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (auto format : formats) {
        VkFormatProperties formatProps;
        vkGetPhysicalDeviceFormatProperties(pdev, format, &formatProps);
        if (tiling == VK_IMAGE_TILING_LINEAR && (formatProps.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (formatProps.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    return VK_FORMAT_UNDEFINED;
}

// transition miplevels of image from the oldl layout to the newl layout
void appvk::transitionImageLayout(VkImage image, VkImageLayout oldl, VkImageLayout newl, unsigned int mipLevels) {
    VkCommandBuffer buf = beginSingleCommand();

    VkImageSubresourceRange range{};

    if (newl == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        range.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (depthFormat == VK_FORMAT_D24_UNORM_S8_UINT || depthFormat == VK_FORMAT_D32_SFLOAT_S8_UINT) {
            range.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    range.levelCount = mipLevels;
    range.layerCount = 1;

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldl;
    barrier.newLayout = newl;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange = range;

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;
    
    if (oldl == VK_IMAGE_LAYOUT_UNDEFINED && newl == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // pseudo-stage that includes vkCopy and vkClear commands, among others

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    } else if (oldl == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newl == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    } else if (oldl == VK_IMAGE_LAYOUT_UNDEFINED && newl == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;

        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    } else {
        throw std::invalid_argument("unsupported stage combination!");
    }

    vkCmdPipelineBarrier(buf, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    endSingleCommand(buf);
}

void appvk::copyBufferToImage(VkBuffer buf, VkImage img, uint32_t width, uint32_t height) {
    VkCommandBuffer cbuf = beginSingleCommand();

    VkImageSubresourceLayers rec{};
    rec.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    rec.mipLevel = 0;
    rec.baseArrayLayer = 0;
    rec.layerCount = 1;

    VkBufferImageCopy copy{};
    copy.bufferOffset = 0;
    copy.imageSubresource = rec;
    copy.imageOffset = {0, 0, 0};
    copy.imageExtent = {width, height, 1};

    vkCmdCopyBufferToImage(cbuf, buf, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copy);

    endSingleCommand(cbuf);
}

void appvk::createImage(unsigned int width, unsigned int height, VkFormat format, unsigned int mipLevels, VkSampleCountFlagBits samples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags props, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    createInfo.imageType = VK_IMAGE_TYPE_2D;
    createInfo.format = format;
    createInfo.extent = {width, height, 1};
    createInfo.mipLevels = mipLevels;
    createInfo.arrayLayers = 1;
    createInfo.samples = samples;
    createInfo.tiling = tiling;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // discard existing texels when loading
    // initiallayout can only be ..._UNDEFINED or ..._PREINITIALIZED
    if (vkCreateImage(dev, &createInfo, nullptr, &image) != VK_SUCCESS) {
        throw std::runtime_error("cannot create texture image!");
    }

    VkMemoryRequirements memReq;
    vkGetImageMemoryRequirements(dev, image, &memReq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memReq.size;
    allocInfo.memoryTypeIndex = findMemoryType(memReq.memoryTypeBits, props);

    if (vkAllocateMemory(dev, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("cannot allocate texture memory!");
    }

    vkBindImageMemory(dev, image, imageMemory, 0);
}

void appvk::createSampler() {
    VkSamplerCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    createInfo.magFilter = VK_FILTER_LINEAR;
    createInfo.minFilter = VK_FILTER_LINEAR;
    createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    createInfo.mipLodBias = 0.0f;
    createInfo.anisotropyEnable = VK_TRUE;
    createInfo.maxAnisotropy = 16.0f;
    createInfo.compareEnable = VK_FALSE;
    createInfo.minLod = 0.0f;
    createInfo.maxLod = texMipLevels;

    if (vkCreateSampler(dev, &createInfo, nullptr, &texSamp) != VK_SUCCESS) {
        throw std::runtime_error("cannot create sampler!");
    }
}

void appvk::generateMipmaps(VkImage image, VkFormat format, unsigned int width, unsigned int height, unsigned int levels) {
    VkCommandBuffer b = beginSingleCommand();

    VkFormatProperties prop;
    vkGetPhysicalDeviceFormatProperties(pdev, format, &prop);
    if (!(prop.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) || !(prop.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT)) {
        throw std::runtime_error("cannot generate mipmaps!");
    }

    VkImageSubresourceRange mipRange;
    mipRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    mipRange.layerCount = 1;
    mipRange.baseArrayLayer = 0;
    mipRange.levelCount = 1;
    
    VkImageMemoryBarrier mipBarrier{};
    mipBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    mipBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mipBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    mipBarrier.image = image;
    mipBarrier.subresourceRange = mipRange;

    VkImageBlit blit{};
    blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.srcSubresource.baseArrayLayer = 0;
    blit.srcSubresource.layerCount = 1;
    blit.srcOffsets[0] = {0, 0, 0};
    blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    blit.dstSubresource.baseArrayLayer = 0;
    blit.dstSubresource.layerCount = 1;
    blit.dstOffsets[0] = {0, 0, 0};

    // turn each mip level from a dest into a source for the one below it
    for (size_t level = 1; level < levels; level++) {
        // move prev dst -> src
        mipBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        mipBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        mipBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        mipBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        mipBarrier.subresourceRange.baseMipLevel = level - 1;
        vkCmdPipelineBarrier(b, 
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
            0, 0, nullptr, 0, nullptr, 1, &mipBarrier);
        
        // blit src -> dst
        blit.srcSubresource.mipLevel = level - 1;
        blit.dstSubresource.mipLevel = level;
        blit.srcOffsets[1].x = (width > 1) ? width : 1;
        blit.srcOffsets[1].y = (height > 1) ? height : 1;
        blit.srcOffsets[1].z = 1;
        blit.dstOffsets[1].x = (width > 1) ? width / 2 : 1;
        blit.dstOffsets[1].y = (height > 1) ? height / 2 : 1;
        blit.dstOffsets[1].z = 1;
        vkCmdBlitImage(b, image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1, &blit, VK_FILTER_LINEAR);

        // src -> shader
        mipBarrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        mipBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        mipBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        mipBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        vkCmdPipelineBarrier(b,
        VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0, 0, nullptr, 0, nullptr, 1, &mipBarrier);

        width /= 2;
        height /= 2;
    }

    // last dst -> shader
    mipBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    mipBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    mipBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    mipBarrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    mipBarrier.subresourceRange.baseMipLevel = levels - 1;
    vkCmdPipelineBarrier(b,
    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
    0, 0, nullptr, 0, nullptr, 1, &mipBarrier);

    endSingleCommand(b);
}