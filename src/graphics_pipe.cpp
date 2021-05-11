#include "main.hpp"

// stores framebuffer config
void appvk::createRenderPass() {
    std::array<VkAttachmentDescription, 3> attachments;

    // multisample
    attachments[0].flags = 0;
    attachments[0].format = swapFormat; // format from swapchain image
    attachments[0].samples = msaaSamples;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // layout of image before render pass - don't care since we'll be clearing it anyways
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // layout of image at end of render pass

    const std::vector<VkFormat> formatList = {
        VK_FORMAT_D24_UNORM_S8_UINT,
        VK_FORMAT_X8_D24_UNORM_PACK32, // no stencil
    };

    // check to see if we can use a 24-bit depth component
    depthFormat = findImageFormat(formatList, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // depth
    attachments[1].flags = 0;
    attachments[1].format = depthFormat;
    attachments[1].samples = msaaSamples; // depth buffer never gets presented, but we want a ms depth buffer to use with our ms color buffer
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // depth has to be cleared to something before we use it
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    // initialLayout needs to be set before we start rendering, otherwise
    // clearing and a layout transition from undefined -> depth stencil optimal happen at the same time
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // resolve
    attachments[2].flags = 0;
    attachments[2].format = swapFormat;
    attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[2].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef;
    colorAttachmentRef.attachment = 0; // index in pAttachments
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // layout to transition to at the start of the subpass

    VkAttachmentReference depthAttachmentRef;
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference resolveAttachmentRef;
    resolveAttachmentRef.attachment = 2;
    resolveAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    std::array<VkSubpassDescription, 1> subs = {};
    subs[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subs[0].colorAttachmentCount = 1; // color attachments are FS outputs, can also specify input / depth attachments, etc.
    subs[0].pColorAttachments = &colorAttachmentRef;
    subs[0].pResolveAttachments = &resolveAttachmentRef;
    subs[0].pDepthStencilAttachment = &depthAttachmentRef;

    std::array<VkSubpassDependency, 1> deps = {};
    // there's a WAW dependency between writing images due to where imageAvailSems waits
    // solution here is to delay writing to the framebuffer until the image we need is acquired (and the transition has taken place)
    
    deps[0].srcSubpass = VK_SUBPASS_EXTERNAL; // implicit subpass at start of render pass
    deps[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // stage we're waiting on
    deps[0].srcAccessMask = 0; // what we're using that input for

    deps[0].dstSubpass = 0; // index into pSubpasses
    deps[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // stage we write to
    deps[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT; // what we're using that output for

    VkRenderPassCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    createInfo.attachmentCount = attachments.size();
    createInfo.pAttachments = attachments.data();
    createInfo.subpassCount = subs.size();
    createInfo.pSubpasses = subs.data();
    createInfo.dependencyCount = deps.size();
    createInfo.pDependencies = deps.data();

    if (vkCreateRenderPass(dev, &createInfo, nullptr, &renderPass) != VK_SUCCESS) {
        throw std::runtime_error("cannot create render pass!");
    }
}

void appvk::createGraphicsPipeline() {
    std::vector<char> vertspv = readFile(".spv/shader.vert.spv");
    std::vector<char> fragspv = readFile(".spv/shader.frag.spv");

    VkShaderModule vmod = createShaderModule(vertspv);
    VkShaderModule fmod = createShaderModule(fragspv);

    std::array<VkPipelineShaderStageCreateInfo, 2> shaders = {};
    
    shaders[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaders[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaders[0].module = vmod;
    shaders[0].pName = "main";
    
    shaders[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaders[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaders[1].module = fmod;
    shaders[1].pName = "main";
    
    VkVertexInputBindingDescription bindDesc;
    bindDesc.binding = 0;
    bindDesc.stride = sizeof(vformat::vertex);
    bindDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    std::array<VkVertexInputAttributeDescription, 4> attrDesc;
    for (size_t i = 0; i < attrDesc.size(); i++) {
        attrDesc[i].location = i;
        attrDesc[i].binding = 0;
        attrDesc[i].offset = 16 * i; // all offsets are rounded up to 16 bytes due to alignas
    }

    attrDesc[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDesc[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDesc[2].format = VK_FORMAT_R32G32_SFLOAT;
    attrDesc[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    
    VkPipelineVertexInputStateCreateInfo vinCreateInfo{};
    vinCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vinCreateInfo.vertexBindingDescriptionCount = 1;
    vinCreateInfo.pVertexBindingDescriptions = &bindDesc;
    vinCreateInfo.vertexAttributeDescriptionCount = attrDesc.size();
    vinCreateInfo.pVertexAttributeDescriptions = attrDesc.data();

    VkPipelineInputAssemblyStateCreateInfo inAsmCreateInfo{};
    inAsmCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inAsmCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inAsmCreateInfo.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = swapExtent.height;
    viewport.width = swapExtent.width;
    // Vulkan says -Y is up, not down, flip so we're compatible with OpenGL code and obj models
    viewport.height = -1.0f * swapExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapExtent;

    VkPipelineViewportStateCreateInfo viewCreateInfo{};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewCreateInfo.viewportCount = 1;
    viewCreateInfo.pViewports = &viewport;
    viewCreateInfo.scissorCount = 1;
    viewCreateInfo.pScissors = &scissor;
    
    VkPipelineRasterizationStateCreateInfo rasterCreateInfo{};
    rasterCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterCreateInfo.depthClampEnable = VK_FALSE; // clamps depth to range instead of discarding it
    rasterCreateInfo.rasterizerDiscardEnable = VK_FALSE; // disables rasterization if true
    rasterCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
    rasterCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // flip cull order due to inverting y in rasterizer
    rasterCreateInfo.depthBiasEnable = VK_FALSE;
    rasterCreateInfo.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo msCreateInfo{};
    msCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    msCreateInfo.sampleShadingEnable = VK_FALSE;
    msCreateInfo.rasterizationSamples = msaaSamples;

    VkPipelineDepthStencilStateCreateInfo dCreateInfo{};
    dCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    dCreateInfo.depthTestEnable = VK_TRUE;
    dCreateInfo.depthWriteEnable = VK_TRUE;
    dCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
    dCreateInfo.depthBoundsTestEnable = VK_FALSE;
    dCreateInfo.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorAttachment{}; // blending information per fb
    colorAttachment.blendEnable = VK_FALSE;
    colorAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | 
                                        VK_COLOR_COMPONENT_G_BIT | 
                                        VK_COLOR_COMPONENT_B_BIT |
                                        VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorCreateInfo{};
    colorCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorCreateInfo.logicOpEnable = VK_FALSE;
    colorCreateInfo.attachmentCount = 1;
    colorCreateInfo.pAttachments = &colorAttachment;
    
    VkPipelineDynamicStateCreateInfo dynCreateInfo{};
    dynCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynCreateInfo.dynamicStateCount = 0;

    VkPipelineLayoutCreateInfo pipeLayoutCreateInfo{}; // for descriptor sets
    pipeLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeLayoutCreateInfo.setLayoutCount = 1;
    pipeLayoutCreateInfo.pSetLayouts = &dSetLayout;

    if (vkCreatePipelineLayout(dev, &pipeLayoutCreateInfo, nullptr, &pipeLayout) != VK_SUCCESS) {
        throw std::runtime_error("cannot create pipeline layout!");
    }

    VkGraphicsPipelineCreateInfo pipeCreateInfo{};
    pipeCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    
    if (shader_debug) {
        pipeCreateInfo.flags = VK_PIPELINE_CREATE_CAPTURE_STATISTICS_BIT_KHR;
    }
    
    pipeCreateInfo.flags |= VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT; // likely want to allow for pipelines to copy this one
    pipeCreateInfo.stageCount = shaders.size();
    pipeCreateInfo.pStages = shaders.data();
    pipeCreateInfo.pVertexInputState = &vinCreateInfo;
    pipeCreateInfo.pInputAssemblyState = &inAsmCreateInfo;
    pipeCreateInfo.pViewportState = &viewCreateInfo;
    pipeCreateInfo.pRasterizationState = &rasterCreateInfo;
    pipeCreateInfo.pMultisampleState = &msCreateInfo;
    pipeCreateInfo.pDepthStencilState = &dCreateInfo;
    pipeCreateInfo.pColorBlendState = &colorCreateInfo;
    pipeCreateInfo.layout = pipeLayout; // handle, not a struct.
    pipeCreateInfo.renderPass = renderPass;
    pipeCreateInfo.subpass = 0;
    
    if (vkCreateGraphicsPipelines(dev, VK_NULL_HANDLE, 1, &pipeCreateInfo, nullptr, &pipe) != VK_SUCCESS) {
        throw std::runtime_error("cannot create graphics pipeline!");
    }

    if (shader_debug && !printed) {
        printShaderStats(pipe);
        printed = true; // prevent stats from being printed again if we recreate the pipeline
    }
    
    vkDestroyShaderModule(dev, vmod, nullptr); // we can destroy shader modules once the graphics pipeline is created.
    vkDestroyShaderModule(dev, fmod, nullptr);
}

void appvk::createFramebuffers() {
    swapFramebuffers.resize(swapImageViews.size());

    for (size_t i = 0; i < swapFramebuffers.size(); i++) {
        
        // having a single depth buffer with >1 swap image only works if graphics and pres queues are the same.
        // this is due to submissions in a single queue having to respect both submission order and semaphores
        VkImageView attachments[] = {
            ms.view, // multisampled render image
            depth.view,
            swapImageViews[i] // swapchain present image
        };

        VkFramebufferCreateInfo fCreateInfo{};
        fCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fCreateInfo.renderPass = renderPass;
        fCreateInfo.attachmentCount = 3;
        fCreateInfo.pAttachments = attachments; // framebuffer attaches to the image view of a swapchain
        fCreateInfo.width = swapExtent.width;
        fCreateInfo.height = swapExtent.height;
        fCreateInfo.layers = 1;

        if (vkCreateFramebuffer(dev, &fCreateInfo, nullptr, &swapFramebuffers[i]) != VK_SUCCESS) {
            throw std::runtime_error("cannot create framebuffer!");
        }
    }
}

appvk::buffer appvk::createVertexBuffer(const std::vector<uint8_t>& verts) {
    VkDeviceSize bufferSize = verts.size();
    buffer staging = createBuffer(verts.size(), 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    buffer local = createBuffer(verts.size(), 
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    void *data;
    vkMapMemory(dev, staging.mem, 0, bufferSize, 0, &data);
    memcpy(data, verts.data(), bufferSize);
    vkUnmapMemory(dev, staging.mem);

    copyBuffer(staging.buf, local.buf, bufferSize);

    vkFreeMemory(dev, staging.mem, nullptr);
    vkDestroyBuffer(dev, staging.buf, nullptr);

    return local;
}

// wrapper for raw createVertexBuffer that takes a vloader mesh
appvk::buffer appvk::createVertexBuffer(std::vector<vformat::vertex>& v) {
    auto bytePtr = reinterpret_cast<uint8_t*>(v.data());
	std::vector<uint8_t> byteData(bytePtr, bytePtr + v.size() * sizeof(vformat::vertex));

    return createVertexBuffer(byteData);
}

appvk::buffer appvk::createIndexBuffer(const std::vector<uint32_t>& indices) {
    VkDeviceSize bufferSize = indices.size() * sizeof(uint32_t);

    buffer staging = createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, 
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    buffer local = createBuffer(bufferSize,
    VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    void *data;
    vkMapMemory(dev, staging.mem, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), bufferSize);
    vkUnmapMemory(dev, staging.mem);

    copyBuffer(staging.buf, local.buf, bufferSize);

    vkFreeMemory(dev, staging.mem, nullptr);
    vkDestroyBuffer(dev, staging.buf, nullptr);

    return local;
}

appvk::texture appvk::createTextureImage(int width, int height, const unsigned char* data, bool makeMips) {

    unsigned int mipLevels;
    if (makeMips) {
        mipLevels = floor(log2(std::max(width, height))) + 1;
    } else {
        mipLevels = 1;
    }
    
    VkDeviceSize imageSize = width * height * 4;

    buffer staging = createBuffer(imageSize, 
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *map_data;
    vkMapMemory(dev, staging.mem, 0, imageSize, 0, &map_data);
    memcpy(map_data, data, imageSize);
    vkUnmapMemory(dev, staging.mem);

    // used as a src when blitting to make mipmaps
    texture t = {createImage(width, height, VK_FORMAT_R8G8B8A8_SRGB, mipLevels, VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
        VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)};
    
    transitionImageLayout(t, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(staging.buf, t.im, uint32_t(width), uint32_t(height));

    vkFreeMemory(dev, staging.mem, nullptr);
    vkDestroyBuffer(dev, staging.buf, nullptr);

    generateMipmaps(t.im, VK_FORMAT_R8G8B8A8_SRGB, width, height, mipLevels);

    return t;
}

void appvk::createDepthImage() {
    depth = createImage(swapExtent.width, swapExtent.height,
        depthFormat, 1, msaaSamples,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    transitionImageLayout(depth, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
    
    depth.view = createImageView(depth.im, depthFormat, 1, VK_IMAGE_ASPECT_DEPTH_BIT);
}

void appvk::createMultisampleImage() {
    ms = createImage(swapExtent.width, swapExtent.height, swapFormat, 1, msaaSamples, 
    VK_IMAGE_TILING_OPTIMAL,
    VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT,
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    ms.view = createImageView(ms.im, swapFormat, 1, VK_IMAGE_ASPECT_COLOR_BIT);
}
