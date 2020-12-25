#include <chrono>
#include <cmath>

#include "main.h"

void appvk::createSyncs() {
    imageAvailSems.resize(framesInFlight, VK_NULL_HANDLE);
    renderDoneSems.resize(framesInFlight, VK_NULL_HANDLE);
    inFlightFences.resize(framesInFlight, VK_NULL_HANDLE);
    imagesInFlight = std::vector<VkFence>(swapImages.size(), VK_NULL_HANDLE); // this needs to be re-created on a window resize
    
    VkSemaphoreCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    VkFenceCreateInfo fCreateInfo{};
    fCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (unsigned int i = 0; i < framesInFlight; i++) {
        VkResult r1 = vkCreateSemaphore(dev, &createInfo, nullptr, &imageAvailSems[i]);
        VkResult r2 = vkCreateSemaphore(dev, &createInfo, nullptr, &renderDoneSems[i]);
        VkResult r3 = vkCreateFence(dev, &fCreateInfo, nullptr, &inFlightFences[i]);
        
        if (r1 != VK_SUCCESS || r2 != VK_SUCCESS || r3 != VK_SUCCESS) {
            throw std::runtime_error("cannot create sync objects!");
        }
    }
}

void appvk::updateUniformBuffer(uint32_t imageIndex) {
    using namespace std::chrono;
    static auto start = high_resolution_clock::now();
    auto current = high_resolution_clock::now();
    float time = duration<float, seconds::period>(current - start).count();

    time = 0;

    // TODO: flip Y axis on .obj models, since the format assumes +Y is up.

    ubo u;
    u.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    u.model = glm::rotate(u.model, time * glm::radians(25.0f), glm::vec3(0.0, 0.0, 1.0));

    // camera flips Y automatically
    u.view = glm::lookAt(c.pos, c.pos + c.front, glm::vec3(0.0f, 1.0f, 0.0f));
    u.proj = glm::perspective(glm::radians(25.0f), swapExtent.width / float(swapExtent.height), 0.1f, 100.0f);

    void* data;
    vkMapMemory(dev, uniformMemories[imageIndex], 0, sizeof(ubo), 0, &data);
    memcpy(data, &u, sizeof(ubo));
    vkUnmapMemory(dev, uniformMemories[imageIndex]);
}

