#include "main.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include "options.hpp"

// seperate from generic ui init so we only rebuild the vulkan part on a window resize
void appvk::initVulkanUI() {	
	// creating a whole new pool of descriptors for imgui
    // (not sure what these correspond to, but set up in example vulkan code...) 
	constexpr size_t poolElems = 11;
	VkDescriptorPoolSize poolSizes[poolElems] =
	{
		{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
		{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
		{ VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
	};

	VkDescriptorPoolCreateInfo poolCreateInfo{};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolCreateInfo.maxSets = 1000 * poolElems;
	poolCreateInfo.poolSizeCount = poolElems;
	poolCreateInfo.pPoolSizes = poolSizes;
	if (vkCreateDescriptorPool(dev, &poolCreateInfo, nullptr, &uiPool) != VK_SUCCESS) {
        throw std::runtime_error("cannot create ui descriptor pool!");
    }

    ImGui_ImplVulkan_InitInfo initInfo{};
    initInfo.Instance = instance;
    initInfo.PhysicalDevice = pdev;
    initInfo.Device = dev;
    initInfo.QueueFamily = gQueueFamily;
    initInfo.Queue = gQueue;
    initInfo.PipelineCache = VK_NULL_HANDLE;
    initInfo.DescriptorPool = uiPool;
    initInfo.Allocator = nullptr;
    initInfo.MinImageCount = 2;
    initInfo.ImageCount = options::framesInFlight;
	initInfo.MSAASamples = getSamples(options::msaaSamples);
    initInfo.CheckVkResultFn = nullptr;
    ImGui_ImplVulkan_Init(&initInfo, renderPass);

    VkCommandBuffer font = beginSingleCommand();
    ImGui_ImplVulkan_CreateFontsTexture(font);
    endSingleCommand(font);
    ImGui_ImplVulkan_DestroyFontUploadObjects();
}