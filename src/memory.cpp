#include "main.hpp"

void appvk::copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size) {
    VkCommandBuffer buf = beginSingleCommand();

    VkBufferCopy copy{};
    copy.size = size;

    vkCmdCopyBuffer(buf, src, dst, 1, &copy);

    endSingleCommand(buf);
}

// find a memory type that our image or buffer can use and that has the properties we want
uint32_t appvk::findMemoryType(uint32_t legalMemoryTypes, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProp{};
    vkGetPhysicalDeviceMemoryProperties(pdev, &memProp);

    // turn a one-hot legalMemoryTypes into an int representing the index we want in memoryTypes
    for (size_t i = 0; i < memProp.memoryTypeCount; i++) {
        // if the type matches one of the allowed types given to us and it has the right flags, return it
        if ((legalMemoryTypes & (1 << i)) && (memProp.memoryTypes[i].propertyFlags & properties)) {
            return i;
        }
    }

    throw std::runtime_error("cannot find proper memory type!");
}

appvk::buffer appvk::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props) {
    VkBufferCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    createInfo.size = size;
    createInfo.usage = usage;
    createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    buffer buf;
    if (vkCreateBuffer(dev, &createInfo, nullptr, &(buf.buf)) != VK_SUCCESS) {
        throw std::runtime_error("cannot create buffer!");
    }

    VkMemoryRequirements mreq{};
    vkGetBufferMemoryRequirements(dev, buf.buf, &mreq);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = mreq.size;
    allocInfo.memoryTypeIndex = findMemoryType(mreq.memoryTypeBits, props);

    if (vkAllocateMemory(dev, &allocInfo, nullptr, &(buf.mem)) != VK_SUCCESS) {
        throw std::runtime_error("cannot allocate buffer memory!");
    }

    vkBindBufferMemory(dev, buf.buf, buf.mem, 0);

    return buf;
}