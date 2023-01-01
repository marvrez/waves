#pragma once

class Device;
class Shader {
public:
    Shader(const Device& device, const char* filename, const char* entrypoint = "main");
    ~Shader();

    std::vector<VkDescriptorSetLayoutBinding> GetLayoutBindings() const { return mLayoutBindings; }
    VkPushConstantRange GetPushConstants() const { return mPushConstants; }
    const char* GetEntrypoint() const { return mEntrypoint; }
    VkShaderModule GetShaderModule() const { return mShaderModule; }
    VkShaderStageFlags GetStage() const { return mStage; }

private:
    const Device& mDevice;
    VkShaderModule mShaderModule = VK_NULL_HANDLE;
    VkShaderStageFlags mStage = VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    const char* mEntrypoint = "main";
    std::vector<VkDescriptorSetLayoutBinding> mLayoutBindings = {};
    VkPushConstantRange mPushConstants = {};
};