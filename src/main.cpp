#include "main.hpp"
#include "extensions.hpp"

#include "vloader.hpp"
#include "iloader.hpp"

#include "options.hpp"

#include <thread>
#include <iomanip>

void appvk::recreateSwapChain() {
	int width, height;
	glfwGetFramebufferSize(w, &width, &height);
	while (width == 0 || height == 0) { // wait until window isn't hidden anymore
		glfwGetFramebufferSize(w, &width, &height);
		glfwWaitEvents(); // put this thread to sleep until events exist
	}

	vkDeviceWaitIdle(dev);

	cleanupSwapChain();

	createSwapChain();
	createSwapViews();
	createRenderPass();
	createGraphicsPipeline();
	createDepthImage();
	createMultisampleImage();
	createFramebuffers();
	createUniformBuffers();
	createDescriptorPool();
	allocDescriptorSets(dPool, descSet, dSetLayout);
	allocDescriptorSetUniform(descSet);
	allocDescriptorSetTexture(descSet, tex);
	allocRenderCmdBuffers();
	createSyncs();
}

appvk::appvk() : basevk(false), c(0.0f, 0.0f, -3.0f) {

	// disable and center cursor
	// glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	vload::vloader g;
	constexpr std::string_view vp = "models/teapot.obj";
	std::thread lv = std::thread([&]() -> void { g.load(vp, true, true); });

	iload::iloader t;
	constexpr std::string_view tp = "textures/grass/grass02 height 1k.jpg";
	std::thread lt = std::thread([&]() -> void { t.load(tp, false); });

	createSurface();
	pickPhysicalDevice(nvidia);
	createLogicalDevice();

	createSwapChain();
	createSwapViews();

	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();

	createCommandPool();
	createDepthImage();
	createMultisampleImage();
	createFramebuffers();

	createUniformBuffers();
	createDescriptorPool();
	allocDescriptorSets(dPool, descSet, dSetLayout);
	allocDescriptorSetUniform(descSet);

	lv.join();

	vert = createVertexBuffer(g.meshList[0].verts);
	index = createIndexBuffer(g.meshList[0].indices);
	cout << "loaded model " << vp << "\n";
	numIndices = g.meshList[0].indices.size();

	lt.join();

	tex = createTextureImage(t.width, t.height, t.data);
	tex.view = createImageView(tex.im, VK_FORMAT_R8G8B8A8_SRGB, tex.mipLevels, VK_IMAGE_ASPECT_COLOR_BIT);
	tex.samp = createSampler(tex.mipLevels);
	cout << "loaded texture " << tp << "\n";

	allocDescriptorSetTexture(descSet, tex);

	allocRenderCmdBuffers();

	createSyncs();
}

void appvk::drawFrame() {

	// NOTE: acquiring an image, writing to it, and presenting it are all async operations.
	// The relevant vulkan calls return before the operation completes.

	// wait for a command buffer to finish writing to the current image
	vkWaitForFences(dev, 1, &inFlightFences[currFrame], VK_FALSE, UINT64_MAX);

	uint32_t nextFrame;
	VkResult r = vkAcquireNextImageKHR(dev, swap, UINT64_MAX, imageAvailSems[currFrame], VK_NULL_HANDLE, &nextFrame);
	// NOTE: currFrame may not always be equal to nextFrame (there's no guarantee that nextFrame increases linearly)

	if (r == VK_ERROR_OUT_OF_DATE_KHR || resizeOccurred) {
		recreateSwapChain(); // have to recreate the swapchain here
		resizeOccurred = false;
		return;
	} else if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) { // we can still technically run with a suboptimal swapchain
		throw std::runtime_error("cannot acquire swapchain image!");
	}

	// wait for the previous frame to finish using the swapchain image at nextFrame
	if (imagesInFlight[nextFrame] != VK_NULL_HANDLE) {
		vkWaitForFences(dev, 1, &imagesInFlight[nextFrame], VK_FALSE, UINT64_MAX);
	}

	imagesInFlight[nextFrame] = inFlightFences[currFrame]; // this frame is using the fence at currFrame

	updateFrame(nextFrame);

	VkSubmitInfo si{};
	si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore renderBeginSems[] = { imageAvailSems[currFrame] };
	VkSemaphore renderEndSems[] = { renderDoneSems[currFrame] };

	si.waitSemaphoreCount = 1;
	si.pWaitSemaphores = renderBeginSems;

	// imageAvailSem waits at this point in the pipeline
	// NOTE: stages not covered by a semaphore may execute before the semaphore is signaled.
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	si.pWaitDstStageMask = waitStages;

	si.commandBufferCount = 1;
	si.pCommandBuffers = &commandBuffers[nextFrame];

	si.signalSemaphoreCount = 1;
	si.pSignalSemaphores = renderEndSems;

	vkResetFences(dev, 1, &inFlightFences[currFrame]); // has to be unsignaled for vkQueueSubmit
	vkQueueSubmit(gQueue, 1, &si, inFlightFences[currFrame]);

	VkPresentInfoKHR pInfo{};
	pInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	pInfo.waitSemaphoreCount = 1;
	pInfo.pWaitSemaphores = renderEndSems;
	pInfo.swapchainCount = 1;
	pInfo.pSwapchains = &swap;
	pInfo.pImageIndices = &nextFrame;

	r = vkQueuePresentKHR(gQueue, &pInfo);
	if (r == VK_ERROR_OUT_OF_DATE_KHR || resizeOccurred) {
		recreateSwapChain();
		resizeOccurred = false;
		return;
	} else if (r != VK_SUCCESS && r != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("cannot submit to queue!");
	}

	currFrame = (currFrame + 1) % options::framesInFlight;
}

void appvk::run() {
	while (!glfwWindowShouldClose(w)) {

		glfwPollEvents();
		c.update(w);
		drawFrame();

		if (glfwGetKey(w, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
			glfwSetWindowShouldClose(w, GLFW_TRUE);
		}
	}

	cout << std::endl;

	vkDeviceWaitIdle(dev);
}

appvk::~appvk() {

    cleanupSwapChain();

    vkDestroyDescriptorSetLayout(dev, dSetLayout, nullptr);

    vkDestroyCommandPool(dev, cp, nullptr);

    vkDestroySampler(dev, tex.samp, nullptr);
    vkDestroyImageView(dev, tex.view, nullptr);
	vkDestroyImage(dev, tex.im, nullptr);
    vkFreeMemory(dev, tex.mem, nullptr);

	vkDestroyBuffer(dev, index.buf, nullptr);
    vkFreeMemory(dev, index.mem, nullptr);

    vkDestroyBuffer(dev, vert.buf, nullptr);
	vkFreeMemory(dev, vert.mem, nullptr);

    vkDestroyDevice(dev, nullptr);
    vkDestroySurfaceKHR(instance, surf, nullptr);
}

int main(int argc, char **argv) {
	(void)argc;
	(void)argv;

	appvk app;
	try {
		app.run();
	} catch (const std::exception& e) {
		cerr << e.what() << "\n";
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
