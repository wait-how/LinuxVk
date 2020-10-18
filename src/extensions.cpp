#include "extensions.h"

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto f = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
	if (f) {
		return f(instance, pCreateInfo, pAllocator, pDebugMessenger);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator) {
	auto f = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (f) {
		f(instance, debugMessenger, pAllocator);
	}
}

VkResult GetPipelineExecutableStatisticsKHR(VkDevice device, const VkPipelineExecutableInfoKHR* pExecutableInfo, uint32_t* pStatisticCount, VkPipelineExecutableStatisticKHR* pStatistics) {
	auto f = (PFN_vkGetPipelineExecutableStatisticsKHR) vkGetDeviceProcAddr(device, "vkGetPipelineExecutableStatisticsKHR");
	if (f) {
		return f(device, pExecutableInfo, pStatisticCount, pStatistics);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

VkResult GetPipelineExecutablePropertiesKHR(VkDevice device, const VkPipelineInfoKHR* pPipelineInfo, uint32_t* pExecutableCount, VkPipelineExecutablePropertiesKHR* pProperties) {
	auto f = (PFN_vkGetPipelineExecutablePropertiesKHR) vkGetDeviceProcAddr(device, "vkGetPipelineExecutablePropertiesKHR");
	if (f) {
		return f(device, pPipelineInfo, pExecutableCount, pProperties);
	} else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}