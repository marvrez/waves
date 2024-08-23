#include "vk/shader.h"

#include "vk/device.h"
#include "vk/common.h"

#include "utils.h"

#include <spirv_reflect.h>

#ifndef SPV_REFLECT_CALL
#define SPV_REFLECT_CALL(call) \
    do { \
        SpvReflectResult res = call; \
        assert(res == SPV_REFLECT_RESULT_SUCCESS); \
    } \
    while (0)
#endif // SPV_REFLECT_CALL

static VkShaderStageFlags GetShaderStage(SpvReflectShaderStageFlagBits reflectShaderStage)
{
    switch (reflectShaderStage) {
        case SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT: return VK_SHADER_STAGE_COMPUTE_BIT;
        case SPV_REFLECT_SHADER_STAGE_VERTEX_BIT: return VK_SHADER_STAGE_VERTEX_BIT;
        case SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT: return VK_SHADER_STAGE_FRAGMENT_BIT;
        default:
            assert(!"Unsupported SpvReflectShaderStageFlagBits!");
            return {};
    }
}

static VkDescriptorType GetDescriptorType(SpvReflectDescriptorType reflectDescriptorType)
{
    switch (reflectDescriptorType) {
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:            return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:             return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:             return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        default:
            assert(!"Unsupported SpvReflectDescriptorType!");
            return {};
    }
}

Shader::Shader(const Device& device, const char* filename, const char* entrypoint)
    : mDevice(device), mEntrypoint(entrypoint)
{
    LOG_INFO("Loading shader '{}' with entrypoint '{}'", filename, entrypoint);
    std::string spvSource = ReadSourceFile(filename);

    const VkShaderModuleCreateInfo shaderModuleCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = uint32_t(spvSource.size()),
        .pCode = (uint32_t*)spvSource.data(),
    };
    VK_CHECK(vkCreateShaderModule(device, &shaderModuleCreateInfo, nullptr, &mShaderModule));

    SpvReflectShaderModule spvModule;
    SPV_REFLECT_CALL(spvReflectCreateShaderModule(shaderModuleCreateInfo.codeSize, shaderModuleCreateInfo.pCode, &spvModule));
    // NOTE: We assume *at most*: 1 entrypoint, 1 descriptor set, 1 push constant block
    assert(spvModule.entry_point_count == 1);
    assert(spvModule.descriptor_set_count <= 1);
    assert(spvModule.push_constant_block_count <= 1);
    mStage = GetShaderStage(spvModule.shader_stage);

    if (spvModule.push_constant_block_count > 0 && spvModule.push_constant_blocks != nullptr) {
        mPushConstants = {
            .stageFlags = mStage,
            .offset = spvModule.push_constant_blocks->offset,
            .size = spvModule.push_constant_blocks->size
         };
    }

    uint32_t spvBindingCount = 0;
    SPV_REFLECT_CALL(spvReflectEnumerateDescriptorBindings(&spvModule, &spvBindingCount, nullptr));
    std::vector<SpvReflectDescriptorBinding*> spvBindings(spvBindingCount);
    SPV_REFLECT_CALL(spvReflectEnumerateDescriptorBindings(&spvModule, &spvBindingCount, spvBindings.data()));

    mLayoutBindings.reserve(spvBindingCount);
    for (int bindingIndex = 0; bindingIndex < spvBindingCount; ++bindingIndex) {
        mLayoutBindings.push_back({
            .binding = spvBindings[bindingIndex]->binding,
            .descriptorCount = 1,
            .descriptorType = GetDescriptorType(spvBindings[bindingIndex]->descriptor_type),
            .stageFlags = mStage,
            .pImmutableSamplers = nullptr,
        });
    }
    spvReflectDestroyShaderModule(&spvModule);
}

Shader::~Shader()
{
    vkDestroyShaderModule(mDevice, mShaderModule, nullptr);
}