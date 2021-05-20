#include <chrono>
#include <cmath>
#include <iomanip>

#include "options.hpp"

#include "main.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"

void appvk::createSyncs() {
    imageAvailSems.resize(options::framesInFlight, VK_NULL_HANDLE);
    renderDoneSems.resize(options::framesInFlight, VK_NULL_HANDLE);
    inFlightFences.resize(options::framesInFlight, VK_NULL_HANDLE);
    imagesInFlight = std::vector<VkFence>(swapImages.size(), VK_NULL_HANDLE); // this needs to be re-created on a window resize
    
    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fCreateInfo{};
    fCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (unsigned int i = 0; i < options::framesInFlight; i++) {
        VkResult r1 = vkCreateSemaphore(dev, &createInfo, nullptr, &imageAvailSems[i]);
        VkResult r2 = vkCreateSemaphore(dev, &createInfo, nullptr, &renderDoneSems[i]);
        VkResult r3 = vkCreateFence(dev, &fCreateInfo, nullptr, &inFlightFences[i]);
        
        if (r1 != VK_SUCCESS || r2 != VK_SUCCESS || r3 != VK_SUCCESS) {
            throw std::runtime_error("cannot create sync objects!");
        }
    }
}

void appvk::updateFrame(uint32_t imageIndex) {
    ubo u;
    // u.model = glm::mat4(1.0f);
    u.model = glm::rotate(glm::mat4(1.0f), glm::radians((float)glfwGetTime() * 20), glm::vec3(1.0f));
    u.view = glm::lookAt(c.pos, c.pos + c.front, glm::vec3(0.0f, 1.0f, 0.0f));
    u.proj = glm::perspective(glm::radians(25.0f), swapExtent.width / float(swapExtent.height), 0.1f, 100.0f);

    void* data;
    vkMapMemory(dev, t.ubos.mem, imageIndex * t.ubos.elemSize, sizeof(ubo), 0, &data);
    memcpy(data, &u, sizeof(ubo));
    vkUnmapMemory(dev, t.ubos.mem);

    u.model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -1.0f, 0.0f));

    vkMapMemory(dev, flr.ubos.mem, imageIndex * t.ubos.elemSize, sizeof(ubo), 0, &data);
    memcpy(data, &u, sizeof(ubo));
    vkUnmapMemory(dev, flr.ubos.mem);

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
		ImGui::Text("camera pos: (%.2f, %.2f, %.2f)", c.pos.x, c.pos.y, c.pos.z);
	}

	ImGui::End(); // must be called regardless of begin() return value

	ImGui::Render();
}
