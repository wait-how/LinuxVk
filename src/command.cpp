#include "main.hpp"

void appvk::createCommandPool() {
    queueIndices qi = findQueueFamily(pdev);
    
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // allow ui command buffers to be reset
    createInfo.queueFamilyIndex = qi.graphics.value();

    if (vkCreateCommandPool(dev, &createInfo, nullptr, &cp) != VK_SUCCESS) {
        throw std::runtime_error("cannot create command pool!");
    }
}

VkCommandBuffer appvk::beginSingleCommand() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = cp;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer buf;

    vkAllocateCommandBuffers(dev, &allocInfo, &buf);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(buf, &beginInfo);

    return buf;
}

void appvk::endSingleCommand(VkCommandBuffer buf) {
    vkEndCommandBuffer(buf);

    VkSubmitInfo subInfo{};
    subInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    subInfo.commandBufferCount = 1;
    subInfo.pCommandBuffers = &buf;

    vkQueueSubmit(gQueue, 1, &subInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(gQueue);

    vkFreeCommandBuffers(dev, cp, 1, &buf);
}

// need to create a command buffer per swapchain image
void appvk::allocRenderCmdBuffers() {
    commandBuffers.resize(swapFramebuffers.size());
    
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = cp;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY; // can't be called from other command buffers, but can be submitted directly to queue
    allocInfo.commandBufferCount = commandBuffers.size();
    
    // we don't actually create command buffers, we create a pool and allocate em.
    if (vkAllocateCommandBuffers(dev, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
        throw std::runtime_error("cannot create command buffers!");
    }

    // we could record command buffers here, but since the ui needs to be re-recorded every frame there isn't much point...?
}