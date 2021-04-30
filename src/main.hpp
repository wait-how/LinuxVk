#pragma once

#include <iostream>
#include <vector>
#include <string_view>
#include <optional> // C++17, for device queue querying
#include <utility> // for std::pair
#include <tuple>

#include "glm_mat_wrapper.hpp"

#include "base.hpp"

#include "vformat.hpp"
#include "camera.hpp"
#include "terrain.hpp"

using std::cout;
using std::cerr;

class appvk : basevk {
public:

	appvk();
	~appvk();

	void run();

private:

	constexpr static bool shader_debug = false;
	
	VkPhysicalDevice pdev = VK_NULL_HANDLE;
    VkSampleCountFlagBits msaaSamples;

	constexpr static std::array<const char*, 2> requiredExtensions = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
		VK_KHR_PIPELINE_EXECUTABLE_PROPERTIES_EXTENSION_NAME,
	};

    enum manufacturer { nvidia, intel, any };

    bool checkDeviceExtensions(VkPhysicalDevice pdev);
    VkSampleCountFlagBits getSamples(unsigned int try_samples);
    void checkChooseDevice(VkPhysicalDevice pd, manufacturer m);
    void pickPhysicalDevice(manufacturer m);

	struct queueIndices {
		std::optional<uint32_t> graphics;
		std::optional<uint32_t> compute;
		std::optional<uint32_t> transfer;
	};

    struct swapChainSupportDetails {
		VkSurfaceCapabilitiesKHR cap;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

    queueIndices findQueueFamily(VkPhysicalDevice pd);
    swapChainSupportDetails querySwapChainSupport(VkPhysicalDevice pdev);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formatList);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& modeList);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& cap);
	
	VkDevice dev = VK_NULL_HANDLE;
	VkQueue gQueue = VK_NULL_HANDLE;
    void createLogicalDevice();
	
	VkSwapchainKHR swap = VK_NULL_HANDLE;
	std::vector<VkImage> swapImages;
	VkFormat swapFormat;
	VkExtent2D swapExtent;
    std::vector<VkImageView> swapImageViews;
    void createSurface();
	void createSwapChain();
    void createSwapViews();

    VkFormat findImageFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features);
    VkImageView createImageView(VkImage im, VkFormat format, unsigned int mipLevels, VkImageAspectFlags aspectMask);
	
	VkRenderPass renderPass = VK_NULL_HANDLE;

    void createRenderPass();

	struct ubo {
		alignas(16) glm::mat4 model;
		alignas(16) glm::mat4 view;
		alignas(16) glm::mat4 proj;
		alignas(16) glm::vec3 eye;
	};

	struct buffer {
		VkBuffer buf = VK_NULL_HANDLE;
		VkDeviceMemory mem = VK_NULL_HANDLE;
	};

	struct image {
		VkImage im = VK_NULL_HANDLE;
		VkDeviceMemory mem = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		unsigned int mipLevels = 0;
	};

	struct texture : image {
		VkSampler samp = VK_NULL_HANDLE;
	};

	std::vector<VkBuffer> mvpBuffers;
	std::vector<VkDeviceMemory> mvpMemories;
	void createUniformBuffers();

    VkDescriptorSetLayout dSetLayout = VK_NULL_HANDLE;
    void createDescriptorSetLayout();

    VkDescriptorPool dPool = VK_NULL_HANDLE;
    void createDescriptorPool();

    std::vector<VkDescriptorSet> descSet;
    void allocDescriptorSets(VkDescriptorPool pool, std::vector<VkDescriptorSet>& dSet, VkDescriptorSetLayout layout);
	void allocDescriptorSetUniform(std::vector<VkDescriptorSet>& dSet);
	void allocDescriptorSetTexture(std::vector<VkDescriptorSet>& dSet, texture tex);

	std::vector<char> readFile(std::string_view path);
    VkShaderModule createShaderModule(const std::vector<char>& spv);
	
	VkPipelineLayout pipeLayout = VK_NULL_HANDLE;
	VkPipeline pipe = VK_NULL_HANDLE;
	void createGraphicsPipeline();

	bool printed = false;
    void printShaderStats(const VkPipeline& pipe);

	std::vector<VkFramebuffer> swapFramebuffers; // ties render attachments to image views in the swapchain
    void createFramebuffers();

	VkCommandPool cp = VK_NULL_HANDLE;
	void createCommandPool();

	uint32_t findMemoryType(uint32_t legalMemoryTypes, VkMemoryPropertyFlags properties);
    buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props);

    VkCommandBuffer beginSingleCommand();
    void endSingleCommand(VkCommandBuffer buf);

	image createImage(unsigned int width, unsigned int height, VkFormat format, unsigned int mipLevels, VkSampleCountFlagBits samples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags props);
    void transitionImageLayout(image image, VkImageLayout oldl, VkImageLayout newl);
    
    void copyBufferToImage(VkBuffer buf, VkImage img, uint32_t width, uint32_t height);
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

	buffer vert;
    buffer createVertexBuffer(std::vector<vformat::vertex>& v);
	buffer createVertexBuffer(const std::vector<uint8_t>& verts);

	buffer index;
	unsigned int numIndices;
    buffer createIndexBuffer(const std::vector<uint32_t>& indices);

	texture tex;
	texture createTextureImage(int width, int height, const uint8_t* data);

    VkSampler createSampler(unsigned int mipLevels);
	void generateMipmaps(VkImage image, VkFormat format, unsigned int width, unsigned int height, unsigned int levels);

	image depth;
	VkFormat depthFormat;
    void createDepthImage();

	image ms;
    void createMultisampleImage();
	
	std::vector<VkCommandBuffer> commandBuffers;
	
	uint32_t grassVertices;
	uint32_t grassIndices;
	void allocRenderCmdBuffers();

	// swapchain image acquisition requires a binary semaphore since it might be hard for implementations to do timeline semaphores
	std::vector<VkSemaphore> imageAvailSems; // use seperate semaphores per frame so we can send >1 frame at once
	std::vector<VkSemaphore> renderDoneSems;
	std::vector<VkFence> inFlightFences; // use fences so we actually wait until a frame completes before moving on to the next one
	std::vector<VkFence> imagesInFlight; // track frames in flight because acquireNextImageKHR may not return swapchain indices in order
    void createSyncs();

    void recreateSwapChain();

	// this scene is set up so that the camera is in -Z looking towards +Z.
    cam::camera c;
	
    void updateFrame(uint32_t imageIndex);

	size_t currFrame = 0;

	void drawFrame();

    void cleanupSwapChain();
};
