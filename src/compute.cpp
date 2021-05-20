#include "main.hpp"

#include <random>

// basic test: create compute buffer to normalize vec4's.
constexpr size_t bufsize = 2048;
std::vector<glm::vec4> hostbuf(bufsize);

void appvk::createComputeBuffers() {
    std::default_random_engine g;
    std::uniform_int_distribution<int> d(-32, 32);

    for (auto& v : hostbuf) {
        v = glm::vec4(d(g), d(g), d(g), d(g));
    }

    ibuf = createBuffer(bufsize * sizeof(glm::vec4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    obuf = createBuffer(bufsize * sizeof(glm::vec4), VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    void* data;
    vkMapMemory(dev, ibuf.mem, 0, bufsize * sizeof(glm::vec4), 0, &data);
    memcpy(data, hostbuf.data(), hostbuf.size() * sizeof(glm::vec4));
    vkUnmapMemory(dev, ibuf.mem);
}

void appvk::createComputeDescriptors() {
    std::array<VkDescriptorSetLayoutBinding, 2> bindings = {};
    bindings[0].binding = 0;
    bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[0].descriptorCount = 1;
    bindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    bindings[1].binding = 1;
    bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    bindings[1].descriptorCount = 1;
    bindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    createInfo.bindingCount = bindings.size();
    createInfo.pBindings = bindings.data();

    if (vkCreateDescriptorSetLayout(dev, &createInfo, nullptr, &cLayout) != VK_SUCCESS) {
        throw std::runtime_error("cannot create compute descriptors!");
    }

    VkDescriptorPoolSize size;
    size.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    size.descriptorCount = 2;

    VkDescriptorPoolCreateInfo poolCreateInfo{};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.maxSets = 1;
    poolCreateInfo.poolSizeCount = 1;
    poolCreateInfo.pPoolSizes = &size;
    
    if (vkCreateDescriptorPool(dev, &poolCreateInfo, nullptr, &cPool) != VK_SUCCESS) {
        throw std::runtime_error("cannot create compute descriptor pool!");
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = cPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &cLayout;
    
    t.dsets.resize(swapImages.size());
    if (vkAllocateDescriptorSets(dev, &allocInfo, &cDescSet) != VK_SUCCESS) {
        throw std::runtime_error("cannot create compute descriptor set!");
    }

    VkDescriptorBufferInfo inBufferInfo{};
    inBufferInfo.buffer = ibuf.buf;
    inBufferInfo.offset = 0;
    inBufferInfo.range = bufsize * sizeof(glm::vec4);

    std::array<VkWriteDescriptorSet, 2> sets = {};
    sets[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    sets[0].dstSet = cDescSet;
    sets[0].dstBinding = 0;
    sets[0].dstArrayElement = 0;
    sets[0].descriptorCount = 1;
    sets[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    sets[0].pBufferInfo = &inBufferInfo;

    VkDescriptorBufferInfo outBufferInfo{};
    outBufferInfo.buffer = obuf.buf;
    outBufferInfo.offset = 0;
    outBufferInfo.range = bufsize * sizeof(glm::vec4);
    
    sets[1] = sets[0];
    sets[1].dstBinding = 1;
    sets[1].pBufferInfo = &outBufferInfo;

    vkUpdateDescriptorSets(dev, 2, sets.data(), 0, nullptr);
}

void appvk::createComputePipeline() {
    std::vector<char> cspv = readFile(".spv/shader.comp.spv");
    VkShaderModule cmod = createShaderModule(cspv);

    VkPipelineShaderStageCreateInfo shaderCreateInfo{};
    shaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderCreateInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderCreateInfo.module = cmod;
    shaderCreateInfo.pName = "main";

    VkPipelineLayoutCreateInfo pipeLayoutCreateInfo{};
    pipeLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLayoutCreateInfo.setLayoutCount = 1;
    pipeLayoutCreateInfo.pSetLayouts = &cLayout;

    if (vkCreatePipelineLayout(dev, &pipeLayoutCreateInfo, nullptr, &cPipeLayout) != VK_SUCCESS) {
        throw std::runtime_error("cannot create compute pipeline layout!");
    }

    VkComputePipelineCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    createInfo.stage = shaderCreateInfo;
    createInfo.layout = cPipeLayout;

    if (vkCreateComputePipelines(dev, VK_NULL_HANDLE, 1, &createInfo, nullptr, &cPipeline) != VK_SUCCESS) {
        throw std::runtime_error("cannot create compute pipeline!");
    }

    vkDestroyShaderModule(dev, cmod, nullptr);
}

VkCommandBuffer appvk::createComputeCommandBuffer() {
    VkCommandPoolCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    createInfo.queueFamilyIndex = cQueueFamily;

    if (vkCreateCommandPool(dev, &createInfo, nullptr, &ccp) != VK_SUCCESS) {
        throw std::runtime_error("cannot create compute command pool!");
    }

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = ccp;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer buf;

    vkAllocateCommandBuffers(dev, &allocInfo, &buf);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(buf, &beginInfo);
        vkCmdBindDescriptorSets(buf, VK_PIPELINE_BIND_POINT_COMPUTE, cPipeLayout, 0, 1, &cDescSet, 0, nullptr);
        vkCmdBindPipeline(buf, VK_PIPELINE_BIND_POINT_COMPUTE, cPipeline);
        vkCmdDispatch(buf, bufsize / 128, 1, 1);
    vkEndCommandBuffer(buf);

    return buf;
}

void appvk::runCompute(VkCommandBuffer buf) {
    VkSubmitInfo subInfo{};
    subInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    subInfo.commandBufferCount = 1;
    subInfo.pCommandBuffers = &buf;

    vkQueueSubmit(cQueue, 1, &subInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(cQueue);

    vkFreeCommandBuffers(dev, ccp, 1, &buf);

    std::vector<glm::vec4> cmpbuf(bufsize);

    void* data;
    vkMapMemory(dev, obuf.mem, 0, bufsize * sizeof(glm::vec4), 0, &data);
    memcpy(cmpbuf.data(), data, cmpbuf.size() * sizeof(glm::vec4));
    vkUnmapMemory(dev, obuf.mem);

    bool err = false;
    for (size_t i = 0; i < bufsize; i++) {
        const glm::vec4 cmpval = glm::vec4(hostbuf[i].x + hostbuf[i].y + hostbuf[i].z + hostbuf[i].w);
        if (cmpbuf[i] != cmpval) {
            err = true;
            cout << "gpu result and host differ at index " << i << "\n";
            break;
        }
    }

    if (!err) {
        cout << "all additions correct!\n";
    }
}