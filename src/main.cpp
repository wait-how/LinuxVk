#include "main.hpp"
#include "extensions.hpp"

#include "vloader.hpp"
#include "iloader.hpp"

#include "options.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

#include <thread>

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

	ImGui_ImplVulkan_Shutdown();
	initVulkanUI();
}

appvk::appvk() : basevk(false), c(0.0f, 0.0f, -3.0f) {

	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 1.5f;

	ImGui_ImplGlfw_InitForVulkan(w, false);

	// disable and center cursor
	// glfwSetInputMode(w, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	vload::vloader obj;
	constexpr std::string_view objstr = "models/teapot.obj";
	std::thread lobj = std::thread([&]() -> void { obj.load(objstr, true, true); });

	iload::iloader t;
	constexpr std::string_view tp = "textures/grass/grass02 height 1k.jpg";
	std::thread lt = std::thread([&]() -> void { t.load(tp, false); });

	createSurface();
	pickPhysicalDevice(any);
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

	lobj.join();

	centerObj.vert = createVertexBuffer(obj.meshList[0].verts);
	centerObj.index = createIndexBuffer(obj.meshList[0].indices);
	cout << "loaded model " << objstr << "\n";
	centerObj.indexCount = obj.meshList[0].indices.size();

	lt.join();

	tex = createTextureImage(t.width, t.height, t.data);
	tex.view = createImageView(tex.im, VK_FORMAT_R8G8B8A8_SRGB, tex.mipLevels, VK_IMAGE_ASPECT_COLOR_BIT);
	tex.samp = createSampler(tex.mipLevels);
	cout << "loaded texture " << tp << "\n";

	allocDescriptorSetTexture(descSet, tex);

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

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	if (ImGui::Begin("demo stats")) {
		// render ui if window is not clipped or hidden for some reason
		using namespace std::chrono;
    	static auto last = high_resolution_clock::now();
    	auto current = high_resolution_clock::now();
    	float time = duration<float, seconds::period>(current - last).count();
    	last = current;

		ImGui::Text("screen dimensions: %dx%d", options::screenWidth, options::screenHeight);
		ImGui::Text("msaa samples: %d", options::msaaSamples);
		ImGui::Text("frame time: %.2f ms (%.2f fps)", time * 1000, 1.0f / time);
	}

	ImGui::End(); // must be called regardless of begin() return value

	ImGui::Render();

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	if (vkBeginCommandBuffer(commandBuffers[nextFrame], &beginInfo) != VK_SUCCESS) {
		throw std::runtime_error("cannot begin recording command buffers!");
	}

	VkRenderPassBeginInfo rBeginInfo{};
	rBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rBeginInfo.renderPass = renderPass;
	rBeginInfo.framebuffer = swapFramebuffers[nextFrame];
	rBeginInfo.renderArea.offset = { 0, 0 };
	rBeginInfo.renderArea.extent = swapExtent;

	VkClearValue attachClearValues[2];
	attachClearValues[0].color = { { 0.15, 0.15, 0.15, 1.0 } };
	attachClearValues[1].depthStencil = {1.0, 0};
	
	rBeginInfo.clearValueCount = 2;
	rBeginInfo.pClearValues = attachClearValues;

	auto& cbuf = commandBuffers[nextFrame];
	
	// commands here respect submission order, but draw command pipeline stages can go out of order
	vkCmdBeginRenderPass(cbuf, &rBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
	
		VkDeviceSize offset[] = { 0 };
		vkCmdBindPipeline(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe);
		vkCmdBindVertexBuffers(cbuf, 0, 1, &centerObj.vert.buf, offset);
		vkCmdBindIndexBuffer(cbuf, centerObj.index.buf, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeLayout, 0, 1, &descSet[nextFrame], 0, nullptr);
		vkCmdDrawIndexed(cbuf, centerObj.indexCount, 1, 0, 0, 0);

		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cbuf);

	vkCmdEndRenderPass(cbuf);
	
	if (vkEndCommandBuffer(commandBuffers[nextFrame]) != VK_SUCCESS) {
		throw std::runtime_error("cannot record into command buffer!");
	}

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

	vkDestroyBuffer(dev, centerObj.index.buf, nullptr);
    vkFreeMemory(dev, centerObj.index.mem, nullptr);

    vkDestroyBuffer(dev, centerObj.vert.buf, nullptr);
	vkFreeMemory(dev, centerObj.vert.mem, nullptr);

	ImGui_ImplVulkan_Shutdown();

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
