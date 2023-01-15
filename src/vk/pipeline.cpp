#include "vk/pipeline.h"

#include "vk/device.h"
#include "vk/shader.h"
#include "vk/common.h"
#include "vk/buffer.h"
#include "vk/texture.h"
#include "vk/descs_conversions.h"

#include "logger.h"
#include "utils.h"

#include <unordered_map>

Binding::Binding(const Buffer& buffer)
{
    bufferInfo = { .buffer = buffer.GetVkBuffer(), .offset = 0u, .range = buffer.GetSizeInBytes() };
}

Binding::Binding(const Texture& texture, VkImageLayout layout)
{
    imageInfo = { .sampler = texture.GetSampler(), .imageView = texture.GetView(), .imageLayout = layout };
}

static bool UpdateBindingStageFlags(std::vector<VkDescriptorSetLayoutBinding>& bindings, VkDescriptorSetLayoutBinding binding)
{
    for (auto& shaderBinding : bindings) {
        if (shaderBinding.binding == binding.binding) {
            shaderBinding.stageFlags |= binding.stageFlags;
            return true;
        }
    }
    return false;
}

static std::vector<VkDescriptorSetLayoutBinding> MergeSetLayoutBindings(const std::vector<Shader*>& shaders)
{
    std::vector<VkDescriptorSetLayoutBinding> mergedLayoutBindings;
    for (const auto& shader : shaders) {
        for (const auto& layoutBinding : shader->GetLayoutBindings()) {
            if (UpdateBindingStageFlags(mergedLayoutBindings, layoutBinding)) {
                continue;
            }
            mergedLayoutBindings.push_back(layoutBinding);
        }
    }
    return mergedLayoutBindings;
}

static VkPushConstantRange MergePushConstants(const std::vector<Shader*>& shaders)
{
    VkPushConstantRange mergedPushConstants = {};
    for (const auto& shader : shaders) {
        VkPushConstantRange pushConstants = shader->GetPushConstants();
        if (pushConstants.stageFlags != 0) {
            if (mergedPushConstants.stageFlags == 0) {
                mergedPushConstants = { .offset = pushConstants.offset, .size = pushConstants.size };
            }
            assert(mergedPushConstants.offset == pushConstants.offset);
            assert(mergedPushConstants.size == pushConstants.size);
            mergedPushConstants.stageFlags |= pushConstants.stageFlags;
        }
    }
    return mergedPushConstants;
}

static VkPipelineBindPoint GetPipelineBindPoint(PipelineType type)
{
    switch (type) {
        case PipelineType::Compute: return VK_PIPELINE_BIND_POINT_COMPUTE;
        case PipelineType::Graphics: return VK_PIPELINE_BIND_POINT_GRAPHICS;
    }
    LOG_ERROR("Unknown pipeline type '{}' was provided", uint32_t(type));
    assert(0);
}

static VkDescriptorSetLayout CreateDescriptorSetLayout(VkDevice device, const std::vector<VkDescriptorSetLayoutBinding>& bindings)
{
    const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_PUSH_DESCRIPTOR_BIT_KHR,
        .bindingCount = uint32_t(bindings.size()),
        .pBindings = bindings.data(),
    };
    VkDescriptorSetLayout descriptorSetLayout;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, nullptr, &descriptorSetLayout));
    return descriptorSetLayout;
}

static VkPipelineLayout CreatePipelineLayout(VkDevice device, VkDescriptorSetLayout setLayout, VkPushConstantRange pushConstants)
{
    const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = 1,
        .pSetLayouts = &setLayout,
        .pushConstantRangeCount = pushConstants.stageFlags != 0 ? 1u : 0u,
        .pPushConstantRanges = pushConstants.stageFlags != 0 ? &pushConstants : nullptr,
    };
    VkPipelineLayout pipelineLayout;
    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout));
    return pipelineLayout;
}

static VkDescriptorUpdateTemplate CreateDescriptorUpdateTemplate(
    VkDevice device,
    VkDescriptorSetLayout setLayout,
    VkPipelineLayout pipelineLayout,
    VkPipelineBindPoint pipelineBindPoint,
    const std::vector<VkDescriptorSetLayoutBinding>& bindings
)
{
    std::vector<VkDescriptorUpdateTemplateEntry> entries;
    for (auto [idx, binding] : enumerate(bindings)) {
        entries.push_back({
            .dstBinding = binding.binding,
            .dstArrayElement = 0,
            .descriptorCount = 1,
            .descriptorType = binding.descriptorType,
            .offset = idx * sizeof(Binding),
            .stride = sizeof(Binding),
        });
    }
    const VkDescriptorUpdateTemplateCreateInfo descriptorUpdateTemplateCreateInfo = { 
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_UPDATE_TEMPLATE_CREATE_INFO,
        .descriptorUpdateEntryCount = uint32_t(entries.size()),
        .pDescriptorUpdateEntries = entries.data(),
        .templateType = VK_DESCRIPTOR_UPDATE_TEMPLATE_TYPE_PUSH_DESCRIPTORS_KHR,
        .descriptorSetLayout = setLayout,
        .pipelineBindPoint = pipelineBindPoint,
        .pipelineLayout = pipelineLayout,
    };
    VkDescriptorUpdateTemplate descriptorUpdateTemplate;
    VK_CHECK(vkCreateDescriptorUpdateTemplate(device, &descriptorUpdateTemplateCreateInfo, nullptr, &descriptorUpdateTemplate));
    return descriptorUpdateTemplate;
}

static VkPipeline CreateComputePipeline(VkDevice device, VkPipelineLayout pipelineLayout, const Shader& shader)
{
    assert(shader.GetStage() == VK_SHADER_STAGE_COMPUTE_BIT);
    const VkPipelineShaderStageCreateInfo shaderStageCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = shader.GetShaderModule(),
        .pName = shader.GetEntrypoint(),
    };
    const VkComputePipelineCreateInfo computePipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = shaderStageCreateInfo,
        .layout = pipelineLayout,
    };
    VkPipeline computePipeline;
    VK_CHECK(vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, nullptr, &computePipeline));
    return computePipeline;
}

struct InputLayout {
    std::vector<VkVertexInputBindingDescription> bindingDesc;
    std::vector<VkVertexInputAttributeDescription> attributeDesc;
};

static InputLayout CreateInputLayout(const std::initializer_list<VertexAttributeDesc> attributeDescs)
{
    InputLayout layout;
    std::unordered_map<uint32_t, VkVertexInputBindingDescription> bindingMap;
    layout.attributeDesc.reserve(attributeDescs.size());
    for (auto [location, desc] : enumerate(attributeDescs)) {
        // collect buffer bindings
        if (!bindingMap.contains(desc.binding)) {
            bindingMap[desc.binding] = {
                .binding = desc.binding,
                .stride = desc.stride,
                .inputRate = desc.isInstanced ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX,
            };
            layout.bindingDesc.push_back(bindingMap[desc.binding]);
        }
        else {
            assert(bindingMap[desc.binding].stride == desc.stride);
            assert(bindingMap[desc.binding].inputRate == (desc.isInstanced ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX));
        }
        // build attribute descriptions
        layout.attributeDesc.push_back({
            .location = uint32_t(location),
            .binding = desc.binding,
            .format = GetVkFormat(desc.format),
            .offset = desc.offset,
        });
    }
    return layout;
}

static VkPipeline CreateGraphicsPipeline(VkDevice device, VkPipelineLayout pipelineLayout, const PipelineDesc& desc)
{
    std::vector<VkPipelineShaderStageCreateInfo> shaderStageCreateInfos;
    shaderStageCreateInfos.reserve(desc.shaders.size());
    for (const auto& shader : desc.shaders) {
        shaderStageCreateInfos.push_back({
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VkShaderStageFlagBits(shader->GetStage()),
            .module = shader->GetShaderModule(),
            .pName = shader->GetEntrypoint(),
        });
    }

    const auto inputLayout = CreateInputLayout(desc.attributeDescs);
    const VkPipelineVertexInputStateCreateInfo vertexInputStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .vertexBindingDescriptionCount = uint32_t(inputLayout.bindingDesc.size()),
        .pVertexBindingDescriptions = inputLayout.bindingDesc.data(),
        .vertexAttributeDescriptionCount = uint32_t(inputLayout.attributeDesc.size()),
        .pVertexAttributeDescriptions = inputLayout.attributeDesc.data(),
    };

    const VkPipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE,
    };

    const VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .scissorCount = 1,
    };

    const VkPipelineRasterizationStateCreateInfo rasterizationStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1.0f,
        .cullMode = GetVkCullModeFlags(desc.rasterization.cullMode),
        .frontFace = GetVkFrontFace(desc.rasterization.cullMode),
        .depthBiasEnable = VK_FALSE,
    };

    const VkPipelineMultisampleStateCreateInfo multisampleStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
    };

    const VkPipelineDepthStencilStateCreateInfo depthStencilStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = desc.depthStencil.shouldEnableDepthTesting,
        .depthWriteEnable = desc.depthStencil.shouldEnableDepthWrite,
        .depthCompareOp = desc.depthStencil.depthCompareOp,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
    };

    std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
    colorBlendAttachments.reserve(desc.attachmentLayout.colorAttachments.size());
    for (auto& colorAttachmentState : desc.attachmentLayout.colorAttachments) {
        colorBlendAttachments.push_back({
            .blendEnable = colorAttachmentState.shouldEnableBlend,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
            .alphaBlendOp = VK_BLEND_OP_ADD,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        });
    }
    const VkPipelineColorBlendStateCreateInfo colorBlendStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = uint32_t(colorBlendAttachments.size()),
        .pAttachments = colorBlendAttachments.data(),
    };

    const std::array<VkDynamicState, 2> dynamicStates = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    const VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = dynamicStates.size(),
        .pDynamicStates = dynamicStates.data(),
    };

    std::vector<VkFormat> colorFormats;
    colorFormats.reserve(desc.attachmentLayout.colorAttachments.size());
    for (auto& colorAttachmentState : desc.attachmentLayout.colorAttachments) {
        colorFormats.push_back(GetVkFormat(colorAttachmentState.format));
    }
    const VkPipelineRenderingCreateInfo pipelineRenderingCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = uint32_t(colorFormats.size()),
        .pColorAttachmentFormats = colorFormats.data(),
        .depthAttachmentFormat = GetVkFormat(desc.attachmentLayout.depthStencilFormat),
    };

    const VkGraphicsPipelineCreateInfo pipelineCreateInfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = uint32_t(shaderStageCreateInfos.size()),
        .pStages = shaderStageCreateInfos.data(),
        .pVertexInputState = &vertexInputStateCreateInfo,
        .pInputAssemblyState = &inputAssemblyStateCreateInfo,
        .pViewportState = &viewportStateCreateInfo,
        .pRasterizationState = &rasterizationStateCreateInfo,
        .pMultisampleState = &multisampleStateCreateInfo,
        .pDepthStencilState = &depthStencilStateCreateInfo,
        .pColorBlendState = &colorBlendStateCreateInfo,
        .pDynamicState = &dynamicStateCreateInfo,
        .layout = pipelineLayout,
        .renderPass = VK_NULL_HANDLE,
        .pNext = &pipelineRenderingCreateInfo,
        .subpass = 0,
        .basePipelineIndex = -1,
        .basePipelineHandle = VK_NULL_HANDLE,
    };
    VkPipeline graphicsPipeline;
    VK_CHECK(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &graphicsPipeline));
    return graphicsPipeline;
}

Pipeline::Pipeline(const Device& device, const PipelineDesc& desc)
    : mDevice(device), mBindPoint(GetPipelineBindPoint(desc.type)), mPushConstants(MergePushConstants(desc.shaders))
{
    const std::vector<VkDescriptorSetLayoutBinding> layoutBindings = MergeSetLayoutBindings(desc.shaders);
    mSetLayout = CreateDescriptorSetLayout(mDevice, layoutBindings);
    mPipelineLayout = CreatePipelineLayout(mDevice, mSetLayout, mPushConstants);
    if (layoutBindings.size() > 0) {
        mUpdateTemplate = CreateDescriptorUpdateTemplate(mDevice, mSetLayout, mPipelineLayout, mBindPoint, layoutBindings);
    }

    if (desc.type == PipelineType::Compute) {
        assert(desc.shaders.size() == 1);
        const auto& shader = *desc.shaders[0];
        mPipeline = CreateComputePipeline(mDevice, mPipelineLayout, shader);
    }
    else if (desc.type == PipelineType::Graphics) {
        mPipeline = CreateGraphicsPipeline(mDevice, mPipelineLayout, desc);
    }
    else {
        LOG_ERROR("Unknown pipeline type '{}' provided", uint32_t(desc.type));
        assert(0);
    }
}

Pipeline::~Pipeline()
{
    vkDestroyPipeline(mDevice, mPipeline, nullptr);
    vkDestroyDescriptorUpdateTemplate(mDevice, mUpdateTemplate, nullptr);
    vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);
    vkDestroyDescriptorSetLayout(mDevice, mSetLayout, nullptr);
}

void Pipeline::Draw(VkCommandBuffer cmdBuf, const DrawDesc& desc, std::function<void()> recordingCallback) const
{
    assert (mBindPoint == VK_PIPELINE_BIND_POINT_GRAPHICS);

    std::vector<VkRenderingAttachmentInfo> colorAttachments;
    colorAttachments.reserve(desc.colorAttachments.size());
    for (const auto& colorAttachment : desc.colorAttachments) {
        assert(colorAttachment.texture->GetImage() != VK_NULL_HANDLE);
        colorAttachments.push_back({
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .imageView = colorAttachment.texture->GetView(),
            .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
            .loadOp = colorAttachment.loadOp,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = { .color = colorAttachment.clear.color },
        });
    }

    VkRenderingInfo renderingInfo = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .layerCount = 1,
        .renderArea = {
            .offset = {
                .x = int32_t(desc.viewport.offset.x),
                .y = int32_t(desc.viewport.offset.y),
            },
            .extent = {
                .width = uint32_t(desc.viewport.extent.x),
                .height = uint32_t(desc.viewport.extent.y),
            },
        },
        .colorAttachmentCount = uint32_t(colorAttachments.size()),
        .pColorAttachments = colorAttachments.data(),
    };

    VkRenderingAttachmentInfo depthAttachment = { VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO };
    if (desc.depthStencilAttachment.texture != nullptr && desc.depthStencilAttachment.texture->GetImage() != VK_NULL_HANDLE) {
        depthAttachment.imageView = desc.depthStencilAttachment.texture->GetView();
        depthAttachment.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL;
        depthAttachment.loadOp = desc.depthStencilAttachment.loadOp;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.clearValue.depthStencil = desc.depthStencilAttachment.clear.depthStencil;

        renderingInfo.pDepthAttachment = &depthAttachment;
    }

    vkCmdBeginRenderingKHR(cmdBuf, &renderingInfo);

    if (desc.viewport.extent.x != 0.0f && desc.viewport.extent.y != 0.0f) {
        const VkViewport viewport = {
            .x = desc.viewport.offset.x, .y = desc.viewport.offset.y,
            .width = desc.viewport.extent.x, .height = desc.viewport.extent.y,
            .minDepth = 0.0f, .maxDepth = 1.0f
        };
        vkCmdSetViewport(cmdBuf, 0u, 1u, &viewport);
    }

    if (desc.scissor.extent.x != 0u && desc.scissor.extent.y != 0u) {
        const VkRect2D scissorRect = {
            .offset = { .x = desc.scissor.offset.x, .y = desc.scissor.offset.y },
            .extent = { .width = desc.scissor.extent.x, .height = desc.scissor.extent.y }
        };
        vkCmdSetScissor(cmdBuf, 0u, 1u, &scissorRect);
    }

    const auto& pushConstants = desc.pushConstants;
    if (pushConstants.byteSize != 0u && pushConstants.data != nullptr) {
        vkCmdPushConstants(
            cmdBuf, mPipelineLayout, mPushConstants.stageFlags, 0u, pushConstants.byteSize, pushConstants.data
        );
    }

    if(desc.bindings.size() > 0) {
        vkCmdPushDescriptorSetWithTemplateKHR(cmdBuf, mUpdateTemplate, mPipelineLayout, 0, desc.bindings.begin());
    }
    vkCmdBindPipeline(cmdBuf, mBindPoint, mPipeline);

    recordingCallback();

    const auto& args = desc.drawArguments;
    // vkCmdDraw(cmdBuf, args.vertexCount, args.instanceCount, args.startVertexLocation, args.startInstanceLocation);

    vkCmdEndRenderingKHR(cmdBuf);
}