#include "main.hpp"
#include "extensions.hpp"

#include "vloader.hpp"
#include "iloader.hpp"

#include "options.hpp"

// config location from inside imgui folder
#define IMGUI_USER_CONFIG "../src/imgui_cfg.hpp"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

void appvk::recreateSwapChain() {
	vkDeviceWaitIdle(dev);

	int width, height;
	glfwGetFramebufferSize(w, &width, &height);
	while (width == 0 || height == 0) { // wait until window isn't hidden anymore
		glfwGetFramebufferSize(w, &width, &height);
		glfwWaitEvents(); // put this thread to sleep until events exist
	}

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

	for (thing& t : things) {
		allocDescriptorSets(dPool, t);
		allocDescriptorSetUniform(t);
	}

	allocDescriptorSetTexture(t, t.diff, 0);
	allocDescriptorSetTexture(t, t.norm, 1);
	allocDescriptorSetTexture(t, t.disp, 2);

	allocDescriptorSetTexture(flr, flr.diff, 0);
	allocDescriptorSetTexture(flr, flr.norm, 1);
	allocDescriptorSetTexture(flr, flr.disp, 2);

	allocRenderCmdBuffers();

	createSyncs();

	ImGui_ImplVulkan_Shutdown();
	initVulkanUI();
}

appvk::appvk() : basevk(false), c(0.0f, 0.0f, -3.0f) {

	IMGUI_CHECKVERSION(); // make sure imgui is set up properly
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 1.5f;

	ImGui_ImplGlfw_InitForVulkan(w, false);

	// disable and center cursor
	// glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	constexpr std::string_view objstr = "models/sphere.obj";
	vload::vloader obj(objstr, true, true, true);
	obj.dispatch();

	constexpr std::string_view fstr = "models/cube.obj";
	vload::vloader f(fstr, true, true, true);
	f.dispatch();

	std::array<iload::iloader, 6> loaders = {
		iload::iloader("textures/grass/diffuse.jpg", false),
		iload::iloader("textures/grass/normal.jpg", false),
		iload::iloader("textures/grass/height.jpg", false),

		iload::iloader("textures/grass2/diffuse.jpg", false),
		iload::iloader("textures/grass2/normal.jpg", false),
		iload::iloader("textures/grass2/height.jpg", false),
	};

	for (auto& ld : loaders) {
		ld.dispatch();
	}

	createSurface();
	pickPhysicalDevice(any);
	createLogicalDevice();

	createComputeBuffers();
	createComputeDescriptors();
	createComputePipeline();
	VkCommandBuffer buf = createComputeCommandBuffer();
	runCompute(buf);

	createSwapChain();
	createSwapViews();

	// for debugging
	t.name = "object";
	flr.name = "floor";

	createRenderPass();
	createDescriptorSetLayout();
	createGraphicsPipeline();

	createCommandPool();
	createDepthImage();
	createMultisampleImage();
	createFramebuffers();

	createUniformBuffers();
	createDescriptorPool();

	for (thing& t : things) {
		allocDescriptorSets(dPool, t);
		allocDescriptorSetUniform(t);
	}

	obj.join();
	t.vert = createVertexBuffer(obj.meshList[0].verts);
	t.index = createIndexBuffer(obj.meshList[0].indices);
	cout << "loaded model " << objstr << "\n";
	t.indices = obj.meshList[0].indices.size();

	f.join();
	flr.vert = createVertexBuffer(f.meshList[0].verts);
	flr.index = createIndexBuffer(f.meshList[0].indices);
	cout << "loaded model " << fstr << "\n\n";
	flr.indices = f.meshList[0].indices.size();

	for (size_t i = 0; i < loaders.size(); i++) {
		loaders[i].join();

		size_t thing_idx = i / 3;
		thing& t = things[thing_idx];

		size_t map_idx = i % 3;
		t.maps[map_idx] = createTextureImage(loaders[i].width, loaders[i].height, loaders[i].data);
		t.maps[map_idx].view = createImageView(t.maps[map_idx].im, VK_FORMAT_R8G8B8A8_SRGB, t.maps[map_idx].mipLevels, VK_IMAGE_ASPECT_COLOR_BIT);
		t.maps[map_idx].samp = createSampler(t.maps[map_idx].mipLevels);

		cout << "loaded texture " << loaders[i].path << "\n";

		allocDescriptorSetTexture(t, t.maps[map_idx], thing_idx);
	}

	allocRenderCmdBuffers();

	createSyncs();

	initVulkanUI();
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

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(commandBuffers[nextFrame], &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("cannot begin recording command buffer!");
	}

	VkRenderPassBeginInfo rBeginInfo{};
	rBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rBeginInfo.renderPass = renderPass;
	rBeginInfo.framebuffer = swapFramebuffers[nextFrame];
	rBeginInfo.renderArea.offset = { 0, 0 };
	rBeginInfo.renderArea.extent = swapExtent;

	std::array<VkClearValue, 2> attachClearValues;
	attachClearValues[0].color = { { 0.15, 0.15, 0.15, 1.0 } };
	attachClearValues[1].depthStencil = {1.0, 0};
	
	rBeginInfo.clearValueCount = attachClearValues.size();
	rBeginInfo.pClearValues = attachClearValues.data();

	auto& cbuf = commandBuffers[nextFrame];
	
	// commands here respect submission order, but draw command pipeline stages can go out of order
	vkCmdBeginRenderPass(cbuf, &rBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	
		VkDeviceSize offset[] = { 0 };

		vkCmdBindPipeline(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, t.pipe);
		vkCmdBindVertexBuffers(cbuf, 0, 1, &t.vert.buf, offset);
		vkCmdBindIndexBuffer(cbuf, t.index.buf, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, t.pipeLayout, 0, 1, &t.dsets[nextFrame], 0, nullptr);
		vkCmdPushConstants(cbuf, t.pipeLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::vec3), &c.pos);
		vkCmdDrawIndexed(cbuf, t.indices, 1, 0, 0, 0);

		vkCmdBindPipeline(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, flr.pipe);
		vkCmdBindVertexBuffers(cbuf, 0, 1, &flr.vert.buf, offset);
		vkCmdBindIndexBuffer(cbuf, flr.index.buf, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, flr.pipeLayout, 0, 1, &flr.dsets[nextFrame], 0, nullptr);
		vkCmdDrawIndexed(cbuf, flr.indices, 1, 0, 0, 0);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cbuf);

	vkCmdEndRenderPass(cbuf);
	
	if (vkEndCommandBuffer(commandBuffers[nextFrame]) != VK_SUCCESS) {
		throw std::runtime_error("cannot record into command buffer!");
	}

	VkSubmitInfo si{};
	si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	si.waitSemaphoreCount = 1;
	si.pWaitSemaphores = &imageAvailSems[currFrame];

	// imageAvailSem waits at this point in the pipeline
	// NOTE: stages not covered by a semaphore may execute before the semaphore is signaled.
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	si.pWaitDstStageMask = waitStages;

	si.commandBufferCount = 1;
	si.pCommandBuffers = &commandBuffers[nextFrame];

	si.signalSemaphoreCount = 1;
	si.pSignalSemaphores = &renderDoneSems[currFrame];

	vkResetFences(dev, 1, &inFlightFences[currFrame]); // has to be unsignaled for vkQueueSubmit
	vkQueueSubmit(gQueue, 1, &si, inFlightFences[currFrame]);

	VkPresentInfoKHR pInfo{};
	pInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	pInfo.waitSemaphoreCount = 1;
	pInfo.pWaitSemaphores = &renderDoneSems[currFrame];
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

	for (thing& t : things) {
		vkDestroyDescriptorSetLayout(dev, t.layout, nullptr);

		for (texture tx : t.maps) {
			vkDestroySampler(dev, tx.samp, nullptr);
			vkDestroyImageView(dev, tx.view, nullptr);
			vkDestroyImage(dev, tx.im, nullptr);
			vkFreeMemory(dev, tx.mem, nullptr);
			tx.mem = VK_NULL_HANDLE; // prevent other frees from failing if all textures allocated together
		}

		vkDestroyBuffer(dev, t.index.buf, nullptr);
		vkFreeMemory(dev, t.index.mem, nullptr);
		t.index.mem = VK_NULL_HANDLE;

		vkDestroyBuffer(dev, t.vert.buf, nullptr);
		vkFreeMemory(dev, t.vert.mem, nullptr);
		t.vert.mem = VK_NULL_HANDLE;
	}

    vkDestroyCommandPool(dev, cp, nullptr);

	ImGui_ImplVulkan_Shutdown();

	vkDestroyCommandPool(dev, ccp, nullptr);

	vkDestroyPipeline(dev, cPipeline, nullptr);
	vkDestroyPipelineLayout(dev, cPipeLayout, nullptr);

	vkDestroyBuffer(dev, ibuf.buf, nullptr);
	vkFreeMemory(dev, ibuf.mem, nullptr);

	vkDestroyBuffer(dev, obuf.buf, nullptr);
	vkFreeMemory(dev, obuf.mem, nullptr);

	vkDestroyDescriptorSetLayout(dev, cLayout, nullptr);
	vkDestroyDescriptorPool(dev, cPool, nullptr);

    vkDestroyDevice(dev, nullptr);
    vkDestroySurfaceKHR(instance, surf, nullptr);

	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
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
