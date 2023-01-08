#include "gui.h"

#include "window.h"
#include "utils.h"

#include "vk/device.h"
#include "vk/texture.h"
#include "vk/buffer.h"
#include "vk/common.h"
#include "vk/shader.h"
#include "vk/pipeline.h"
#include "vk/swapchain.h"

#include <imgui.h>

struct PushConstantData {
    glm::vec2 scale;
    glm::vec2 translate;
};

GUI::GUI(const Device& device, const Swapchain& swapchain, const Window& window)
    : mDevice(device), mWindow(window), mSwapchain(swapchain)
{
    const auto [windowWidth, windowHeight] = window.GetWindowSize();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.DisplaySize = ImVec2(windowWidth, windowHeight);
    io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);

    ImGuiStyle& style = ImGui::GetStyle();
    style.FrameRounding = 5.0f;
    style.WindowRounding = 7.0f;
    style.WindowBorderSize = 2.0f;

    this->CreateFontTexture();

    Shader imguiVS = Shader(device, "imgui.vs.spv");
    Shader imguiPS = Shader(device, "imgui.ps.spv");
    const PipelineDesc pipelineDesc = {
        .type = PipelineType::Graphics,
        .shaders = { &imguiVS, &imguiPS },
        .attachmentLayout = {
            .colorAttachments = {{
                .format = swapchain.GetFormat(),
                .shouldEnableBlend = true
            }},
        },
        .rasterization = {
            .cullMode = VK_CULL_MODE_NONE,
            .frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE
        },
        .depthStencil = { .depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL },
        .attributeDescs = {
            { .name = "POSITION0", .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(ImDrawVert, pos), .stride = sizeof(ImDrawVert) },
            { .name = "TEXCOORD0", .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(ImDrawVert, uv), .stride = sizeof(ImDrawVert) },
            { .name = "COLOR0", .format = VK_FORMAT_R8G8B8A8_UNORM, .offset = offsetof(ImDrawVert, col), .stride = sizeof(ImDrawVert) },
        },
    };
    mPipeline = std::make_unique<Pipeline>(device, pipelineDesc);
}

void GUI::CreateFontTexture()
{
    uint8_t* fontData;
    int textureWidth, textureHeight, bytesPerPixel;
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->GetTexDataAsRGBA32(&fontData, &textureWidth, &textureHeight, &bytesPerPixel);

    const TextureDesc texDesc = {
        .width = uint32_t(textureWidth),
        .height = uint32_t(textureHeight),
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .samplerDesc = {
            .filterMode = VK_FILTER_LINEAR,
            .addressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
            .mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR
        }
    };
    mFontTexture = std::make_unique<Texture>(mDevice, texDesc);

    const VkDeviceSize uploadSizeInBytes = textureWidth * textureHeight * bytesPerPixel;
    Buffer stagingBuffer = Buffer(mDevice, {
        .byteSize = uploadSizeInBytes,
        .access = MemoryAccess::HOST,
        .usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT
    });
    memcpy(stagingBuffer.GetMappedData(), fontData, uploadSizeInBytes);

    mDevice.Submit([&](VkCommandBuffer cmdBuf) {
        mFontTexture->RecordBarrier(cmdBuf,
            VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            VK_ACCESS_NONE, VK_ACCESS_TRANSFER_WRITE_BIT,
            VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT
        );

        const VkBufferImageCopy bufferCopyRegion{
            .imageSubresource = { .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT, .layerCount = 1 },
            .imageExtent = { .width = uint32_t(textureWidth), .height = uint32_t(textureHeight), .depth = 1 }
        };
        vkCmdCopyBufferToImage(
            cmdBuf,
            stagingBuffer.GetVkBuffer(),
            mFontTexture->GetImage(),
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &bufferCopyRegion
        );

        mFontTexture->RecordBarrier(cmdBuf,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
            VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
        );
    });
}

GUI::~GUI()
{
    mDevice.WaitIdle();
    ImGui::DestroyContext();
}

void GUI::NewFrame()
{
    // Setup IO
    ImGuiIO& io = ImGui::GetIO();

    const auto [windowWidth, windowHeight] = mWindow.GetWindowSize();
    const auto [framebufferWidth, framebufferHeight] = mWindow.GetFramebufferSize();
    io.DisplaySize = ImVec2(windowWidth, windowHeight);
    if (windowWidth > 0 && windowHeight > 0) {
        io.DisplayFramebufferScale = ImVec2(
            float(framebufferWidth)  / float(windowWidth),
            float(framebufferHeight) / float(windowHeight)
        );
    }

    static double previousTime = 0.0;
    double currentTime = glfwGetTime();
    io.DeltaTime = float(currentTime - previousTime);
    previousTime = currentTime;

    const auto [xMousePosition, yMousePosition] = mWindow.GetCursorPosition();
    io.MousePos = ImVec2(xMousePosition, yMousePosition);
    io.MouseDown[0] = mWindow.IsMouseLeftButtonPressed();
    io.MouseDown[1] = mWindow.IsMouseRightButtonPressed();

    // Now, render the actual GUI
    ImGui::NewFrame();
    // TODO: REPLACE WITH ACTUAL GUI STUFF
    ImGui::ShowDemoWindow();
    ImGui::Render();
}

void GUI::UpdateBuffers(uint32_t frameIndex)
{
    ImDrawData* drawData = ImGui::GetDrawData();

    const uint64_t vertexBufferSize = drawData->TotalVtxCount * sizeof(ImDrawVert);
    const uint64_t indexBufferSize  = drawData->TotalIdxCount * sizeof(ImDrawIdx);
    if (vertexBufferSize == 0 || indexBufferSize == 0) return;

    auto& vertexBuffer = mVertexBuffers[frameIndex];
    auto& indexBuffer = mIndexBuffers[frameIndex];
    if (vertexBuffer == nullptr || vertexBuffer->GetVkBuffer() == VK_NULL_HANDLE || vertexBuffer->GetSizeInBytes() < vertexBufferSize) {
        const BufferDesc desc = {
            .byteSize = GetAlignedSize(vertexBufferSize, 32'000ull),
            .access = MemoryAccess::HOST,
            .usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
        };
        vertexBuffer = std::make_unique<Buffer>(mDevice, desc);
    }
    if (indexBuffer == nullptr || indexBuffer->GetVkBuffer() == VK_NULL_HANDLE || indexBuffer->GetSizeInBytes() < indexBufferSize) {
        const BufferDesc desc = {
            .byteSize = GetAlignedSize(indexBufferSize, 16'000ull),
            .access = MemoryAccess::HOST,
            .usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT
        };
        indexBuffer = std::make_unique<Buffer>(mDevice, desc);
    }

    ImDrawVert* vertexData = (ImDrawVert*)vertexBuffer->GetMappedData();
    ImDrawIdx* indexData = (ImDrawIdx*)indexBuffer->GetMappedData();
    for (int listIdx = 0; listIdx < drawData->CmdListsCount; ++listIdx) {
        const ImDrawList* cmdList = drawData->CmdLists[listIdx];
        memcpy(vertexData, cmdList->VtxBuffer.Data, cmdList->VtxBuffer.Size * sizeof(ImDrawVert));
        memcpy(indexData, cmdList->IdxBuffer.Data, cmdList->IdxBuffer.Size * sizeof(ImDrawIdx));
        vertexData += cmdList->VtxBuffer.Size;
        indexData += cmdList->IdxBuffer.Size;
    }
}

void GUI::DrawFrame(VkCommandBuffer cmdBuf, uint32_t swapchainImageIndex, uint32_t frameIndex)
{
    this->UpdateBuffers(frameIndex);
    const auto& vertexBuffer = mVertexBuffers[frameIndex];
    const auto& indexBuffer  = mIndexBuffers[frameIndex];
    if (vertexBuffer == nullptr || indexBuffer == nullptr) return;

    ImGuiIO& io = ImGui::GetIO();
    const PushConstantData pushConstantData = {
        .scale = glm::vec2(2.0f / io.DisplaySize.x, 2.0f / io.DisplaySize.y),
        .translate = glm::vec2(-1.0f)
    };

    const auto& tex = mSwapchain.GetTexture(swapchainImageIndex);
    const DrawDesc drawDesc = {
        .viewport = { .offset = { 0.0f, 0.0f }, .extent = { io.DisplaySize.x, io.DisplaySize.y } },
        .colorAttachments = {{ .texture = &tex, .loadOp = VK_ATTACHMENT_LOAD_OP_LOAD }},
        .bindings = { Binding(*mFontTexture, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) },
        .pushConstants = { .byteSize = sizeof(PushConstantData), .data = (void*)&pushConstantData }
    };
    mPipeline->Draw(cmdBuf, drawDesc, [&]() {
        ImDrawData* drawData = ImGui::GetDrawData();
        int globalIndexOffset = 0, globalVertexOffset = 0;
        if (drawData->CmdListsCount > 0) {
            const VkBuffer vertexBuffers[] = { vertexBuffer->GetVkBuffer() };
            const VkDeviceSize offsets[] = { 0 };
            vkCmdBindVertexBuffers(cmdBuf, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(cmdBuf, indexBuffer->GetVkBuffer(), 0, VK_INDEX_TYPE_UINT16);
            for (int listIdx = 0; listIdx < drawData->CmdListsCount; ++listIdx) {
                const ImDrawList* cmdList = drawData->CmdLists[listIdx];
                for (int bufIdx = 0; bufIdx < cmdList->CmdBuffer.Size; ++bufIdx) {
                    const ImDrawCmd* drawCmd = &cmdList->CmdBuffer[bufIdx];
                    const VkRect2D scissorRect = {
                        .offset = {
                            .x = std::max(int(drawCmd->ClipRect.x), 0),
                            .y = std::max(int(drawCmd->ClipRect.y), 0)
                        },
                        .extent = {
                            .width = uint32_t(drawCmd->ClipRect.z - drawCmd->ClipRect.x),
                            .height = uint32_t(drawCmd->ClipRect.w - drawCmd->ClipRect.y) 
                        }
                    };
                    vkCmdSetScissor(cmdBuf, 0, 1, &scissorRect);
                    vkCmdDrawIndexed(cmdBuf, drawCmd->ElemCount, 1, drawCmd->IdxOffset + globalIndexOffset, drawCmd->VtxOffset + globalVertexOffset, 0);
                }
                globalIndexOffset += cmdList->IdxBuffer.Size;
                globalVertexOffset += cmdList->VtxBuffer.Size;
            }
        }
    });
}