#include <cstdint> // for UINT32_MAX

#include "main.h"

void appvk::createSurface() {
    // platform-agnostic version of vulkan create surface extension
    if (glfwCreateWindowSurface(instance, w, nullptr, &surf) != VK_SUCCESS) {
        throw std::runtime_error("cannot create window surface!");
    }
}

appvk::swapChainSupportDetails appvk::querySwapChainSupport(VkPhysicalDevice pdev) {
    swapChainSupportDetails d;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pdev, surf, &d.cap);

    uint32_t numFormats;
    vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surf, &numFormats, nullptr);
    
    if (numFormats > 0) {
        d.formats.resize(numFormats);
        vkGetPhysicalDeviceSurfaceFormatsKHR(pdev, surf, &numFormats, d.formats.data());
    }

    uint32_t numPresentModes;
    vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surf, &numPresentModes, nullptr);
    
    if (numPresentModes > 0) {
        d.presentModes.resize(numPresentModes);
        vkGetPhysicalDeviceSurfacePresentModesKHR(pdev, surf, &numPresentModes, d.presentModes.data());
    }
    
    return d;
}

VkSurfaceFormatKHR appvk::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formatList) {
    for (const auto& format : formatList) {
        // RGBA8 is pretty standard
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            return format;
        }
    }

    return formatList[0];
}

VkPresentModeKHR appvk::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modeList) {
    for (const auto& mode : modeList) {
        if (mode == VK_PRESENT_MODE_MAILBOX_KHR) { // triple buffer if possible
            return mode;
        }
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D appvk::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& cap) {
    if (cap.currentExtent.width != UINT32_MAX) {
        return cap.currentExtent; // if the surface has a width and height already, use that
    } else {
        VkExtent2D newV;
        // clamp width and height to [min, max] extent height
        
        glfwGetFramebufferSize(w, (int*)&screenWidth, (int*)&screenHeight);
        
        newV.width = std::max(cap.minImageExtent.width, std::min(cap.maxImageExtent.width, static_cast<uint32_t>(screenWidth)));
        newV.height = std::max(cap.minImageExtent.height, std::min(cap.minImageExtent.height, static_cast<uint32_t>(screenHeight)));
        return newV;
    }
}

void appvk::createSwapChain() {
    swapChainSupportDetails sdet = querySwapChainSupport(pdev);

    VkSurfaceFormatKHR f = chooseSwapSurfaceFormat(sdet.formats);
    VkPresentModeKHR p = chooseSwapPresentMode(sdet.presentModes);
    VkExtent2D e = chooseSwapExtent(sdet.cap);
    
    uint32_t numImages = sdet.cap.minImageCount + 1; // perf improvement - don't have to wait for the driver to complete stuff to continue rendering
    numImages = std::max(numImages, sdet.cap.maxImageCount);

    VkSwapchainCreateInfoKHR sInfo{};
    sInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;

    sInfo.surface = surf;
    sInfo.minImageCount = numImages;
    sInfo.imageFormat = f.format; // swapchain creates images for us
    sInfo.imageExtent = e;
    sInfo.imageColorSpace = f.colorSpace;
    sInfo.imageArrayLayers = 1; // no stereoscopic viewing
    sInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    sInfo.presentMode = p;
    sInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    // if the presentation and graphics queues are different, then both have to access the swapchain and we have to set that behavior here
    sInfo.preTransform = sdet.cap.currentTransform;
    sInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sInfo.clipped = VK_TRUE;
    sInfo.oldSwapchain = VK_NULL_HANDLE;

    if (vkCreateSwapchainKHR(dev, &sInfo, nullptr, &swap) != VK_SUCCESS) {
        throw std::runtime_error("unable to create swapchain!");
    }
    
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(dev, swap, &imageCount, nullptr);
    swapImages.resize(imageCount);
    vkGetSwapchainImagesKHR(dev, swap, &imageCount, swapImages.data());
    
    swapFormat = f.format;
    swapExtent = e;
}

void appvk::createSwapViews() {
    swapImageViews.resize(swapImages.size());
    for (size_t i = 0; i < swapImages.size(); i++) {
        swapImageViews[i] = createImageView(swapImages[i], swapFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

void appvk::cleanupSwapChain() {

    for (unsigned int i = 0; i < framesInFlight; i++){
        vkDestroySemaphore(dev, imageAvailSems[i], nullptr);
        vkDestroySemaphore(dev, renderDoneSems[i], nullptr);
        vkDestroyFence(dev, inFlightFences[i], nullptr);
    }

    vkFreeCommandBuffers(dev, cp, commandBuffers.size(), commandBuffers.data());

    vkDestroyImageView(dev, depthView, nullptr);
    vkFreeMemory(dev, depthMemory, nullptr);
    vkDestroyImage(dev, depthImage, nullptr);

    vkDestroyImageView(dev, msImageView, nullptr);
    vkFreeMemory(dev, msMemory, nullptr);
    vkDestroyImage(dev, msImage, nullptr);

    for (size_t i = 0; i < swapImages.size(); i++) {
        vkFreeMemory(dev, uniformMemories[i], nullptr);
        vkDestroyBuffer(dev, uniformBuffers[i], nullptr);
    }

    vkDestroyDescriptorPool(dev, dPool, nullptr);

    for (auto framebuffer : swapFramebuffers) {
        vkDestroyFramebuffer(dev, framebuffer, nullptr);
    }

    vkDestroyPipeline(dev, gpipe, nullptr);
    vkDestroyPipelineLayout(dev, pipeLayout, nullptr);
    vkDestroyRenderPass(dev, renderPass, nullptr);
    
    for (const auto& view : swapImageViews) {
        vkDestroyImageView(dev, view, nullptr);
    }

    vkDestroySwapchainKHR(dev, swap, nullptr);
}