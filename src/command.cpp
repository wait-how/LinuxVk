#include "main.hpp"

void appvk::createCommandPool() {
    queueIndices qi = findQueueFamily(pdev);
    
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
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

    for (size_t i = 0; i < commandBuffers.size(); i++) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        if (vkBeginCommandBuffer(commandBuffers[i], &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("cannot begin recording command buffers!");
        }

        VkRenderPassBeginInfo rBeginInfo{};
        rBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rBeginInfo.renderPass = renderPass;
        rBeginInfo.framebuffer = swapFramebuffers[i];
        rBeginInfo.renderArea.offset = { 0, 0 };
        rBeginInfo.renderArea.extent = swapExtent;

        VkClearValue attachClearValues[2];
        attachClearValues[0].color = { { 0.15, 0.15, 0.15, 1.0 } };
        attachClearValues[1].depthStencil = {1.0, 0};
        
        rBeginInfo.clearValueCount = 2;
        rBeginInfo.pClearValues = attachClearValues;

        auto& cbuf = commandBuffers[i];
        
        // commands here respect submission order, but draw command pipeline stages can go out of order
        vkCmdBeginRenderPass(cbuf, &rBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
        
            VkDeviceSize offset[] = { 0 };
            vkCmdBindPipeline(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
            vkCmdBindVertexBuffers(cbuf, 0, 1, &vert.buf, offset);
            vkCmdBindIndexBuffer(cbuf, index.buf, 0, VK_INDEX_TYPE_UINT32);
            vkCmdBindDescriptorSets(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeLayout, 0, 1, &descSet[i], 0, nullptr);
            vkCmdDrawIndexed(cbuf, numIndices, 1, 0, 0, 0);

        vkCmdEndRenderPass(cbuf);
        
        if (vkEndCommandBuffer(commandBuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("cannot record into command buffer!");
        }
    }
}