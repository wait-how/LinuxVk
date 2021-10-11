#include "extensions.hpp"
#include "main.hpp"

#include <fstream>
#include <string>

std::vector<char> appvk::readFile(std::string_view path) {
    std::ifstream file(path.data(), std::ios::ate | std::ios::binary);

    if (!file) {
        throw std::runtime_error(std::string("cannot open file ") + path.data() + "!");
    }
    
    size_t len = static_cast<size_t>(file.tellg());
    file.seekg(0);
    
    std::vector<char> contents(len); // using vector of char because spir-v isn't a string
    file.read(contents.data(), len);
    
    file.close();

    return contents;
}

VkShaderModule appvk::createShaderModule(const std::vector<char>& spv) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = spv.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(spv.data());

    VkShaderModule mod;
    if (vkCreateShaderModule(dev, &createInfo, nullptr, &mod) != VK_SUCCESS) {
        throw std::runtime_error("cannot create shader module!");
    }
    return mod;
}

void appvk::printShaderStats(const thing& t) {
    VkPipelineInfoKHR pipeInfo{};
    pipeInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INFO_KHR;
    pipeInfo.pipeline = t.pipe;

    unsigned int numShaders;
    if (GetPipelineExecutablePropertiesKHR(dev, &pipeInfo, &numShaders, nullptr) != VK_SUCCESS) {
        throw std::runtime_error("cannot get shader statistics!");
    }

    // get stats for every shader in the pipeline
    std::vector<VkPipelineExecutablePropertiesKHR> shaderProps(numShaders);
    for (auto& prop : shaderProps) {
        prop.sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_PROPERTIES_KHR;
    }
    GetPipelineExecutablePropertiesKHR(dev, &pipeInfo, &numShaders, shaderProps.data());
    
    VkPipelineExecutableInfoKHR shaderInfo{};
    shaderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_INFO_KHR;
    shaderInfo.pipeline = t.pipe;
    
    unsigned int numStats;
    GetPipelineExecutableStatisticsKHR(dev, &shaderInfo, &numStats, nullptr);
    std::vector<VkPipelineExecutableStatisticKHR> shaderStats(numStats);
    for (auto& stat : shaderStats) {
        stat.sType = VK_STRUCTURE_TYPE_PIPELINE_EXECUTABLE_STATISTIC_KHR;
    }
    
    for (size_t i = 0; i < numShaders; i++) {
        GetPipelineExecutableStatisticsKHR(dev, &shaderInfo, &numStats, shaderStats.data());

        cout << t.name << " " << shaderProps[i].name << " statistics:\n";
        
        for (auto stat : shaderStats) {
            cout << "  ~ " << stat.name << " = ";
            switch (stat.format) {
                case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_BOOL32_KHR:
                    cout << stat.value.b32;
                    break;
                case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_INT64_KHR:
                    cout << stat.value.i64;
                    break;
                case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_UINT64_KHR:
                    cout << stat.value.u64;
                    break;
                case VK_PIPELINE_EXECUTABLE_STATISTIC_FORMAT_FLOAT64_KHR:
                    cout << stat.value.f64;
                    break;
                default:
                    break;
            }
            cout << "\n";
        }

        cout << "  ~ Subgroup Size: " << shaderProps[i].subgroupSize << "\n";
        shaderInfo.executableIndex++;
    }
}