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
	uint32_t gQueueFamily;
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

	struct bufslab {
		std::vector<VkBuffer> bufs;
		VkDeviceMemory mem = VK_NULL_HANDLE;
		VkDeviceSize elemSize = 0; // _actual_ size of a buffer in mem (due to GPU memory alignment)
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

	struct thing {
		buffer vert;
		buffer index;
		unsigned int indices = 0;

		std::array<texture, 3> maps;
		texture& diff = maps[0];
		texture& norm = maps[1];
		texture& disp = maps[2];

		VkDescriptorSetLayout layout = VK_NULL_HANDLE;
		std::vector<VkDescriptorSet> dsets;
		bufslab ubos;

		VkPipelineLayout pipeLayout = VK_NULL_HANDLE;
		VkPipeline pipe = VK_NULL_HANDLE;
	};

	void createUniformBuffers();    
    void createDescriptorSetLayout();

    VkDescriptorPool dPool = VK_NULL_HANDLE;
	VkDescriptorPool uiPool = VK_NULL_HANDLE;
    void createDescriptorPool();

    void allocDescriptorSets(VkDescriptorPool pool, thing& t);
	void allocDescriptorSetUniform(thing& t);
	void allocDescriptorSetTexture(thing& t, texture tex, size_t index);

	std::vector<char> readFile(std::string_view path);
    VkShaderModule createShaderModule(const std::vector<char>& spv);
	
	void createGraphicsPipeline();

	bool printed = false;
    void printShaderStats(const VkPipeline& pipe);

	std::vector<VkFramebuffer> swapFramebuffers; // ties render attachments to image views in the swapchain
    void createFramebuffers();

	VkCommandPool cp = VK_NULL_HANDLE;
	void createCommandPool();

	uint32_t findMemoryType(uint32_t legalMemoryTypes, VkMemoryPropertyFlags properties);
    buffer createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props);
	bufslab createBuffers(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags props, unsigned int count);

    VkCommandBuffer beginSingleCommand();
    void endSingleCommand(VkCommandBuffer buf);

	image createImage(unsigned int width, unsigned int height, VkFormat format, unsigned int mipLevels,
		VkSampleCountFlagBits samples, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags props);
    void transitionImageLayout(image image, VkImageLayout oldl, VkImageLayout newl);
    
    void copyBufferToImage(VkBuffer buf, VkImage img, uint32_t width, uint32_t height);
    void copyBuffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);

	std::array<thing, 2> things;
	thing& t = things[0];
	thing& flr = things[1];

    buffer createVertexBuffer(std::vector<vformat::vertex>& v);
	buffer createVertexBuffer(const std::vector<uint8_t>& verts);

    buffer createIndexBuffer(const std::vector<uint32_t>& indices);

	texture createTextureImage(int width, int height, const uint8_t* data, bool makeMips = true);

    VkSampler createSampler(unsigned int mipLevels);
	void generateMipmaps(VkImage image, VkFormat format, unsigned int width, unsigned int height, unsigned int levels);

	image depth;
	VkFormat depthFormat;
    void createDepthImage();

	image ms;
    void createMultisampleImage();
	
	std::vector<VkCommandBuffer> commandBuffers;
	
	void allocRenderCmdBuffers();

	// swapchain image acquisition requires a binary semaphore since it might be hard for implementations to do timeline semaphores
	std::vector<VkSemaphore> imageAvailSems; // use seperate semaphores per frame so we can send >1 frame at once
	std::vector<VkSemaphore> renderDoneSems;
	std::vector<VkFence> inFlightFences; // use fences so we actually wait until a frame completes before moving on to the next one
	std::vector<VkFence> imagesInFlight; // track frames in flight because acquireNextImageKHR may not return swapchain indices in order
    void createSyncs();

	void initVulkanUI();
	
    void recreateSwapChain();

	// this scene is set up so that the camera is in -Z looking towards +Z.
    cam::camera c;
	
    void updateFrame(uint32_t imageIndex);

	uint32_t currFrame = 0;

	void drawFrame();

    void cleanupSwapChain();
};
