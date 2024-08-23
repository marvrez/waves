#include "gui.h"

#include "window.h"
#include "utils.h"

#include "vk/device.h"
#include "vk/texture.h"
#include "vk/command_list.h"
#include "vk/buffer.h"
#include "vk/common.h"
#include "vk/shader.h"
#include "vk/pipeline.h"
#include "vk/swapchain.h"
#include "vk/descs.h"

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
        .type = PipelineType::GRAPHICS,
        .shaders = { &imguiVS, &imguiPS },
        .attachmentLayout = {
            .colorAttachments = {{
                .format = swapchain.GetFormat(),
                .shouldEnableBlend = true
            }},
        },
        .rasterization = { .cullMode = CullMode::NONE },
        .depthStencil = { .depthCompareOp = CompareOp::LESS_OR_EQUAL },
        .attributeDescs = {
            { .name = "POSITION0", .format = Format::RG32_FLOAT, .offset = offsetof(ImDrawVert, pos), .stride = sizeof(ImDrawVert) },
            { .name = "TEXCOORD0", .format = Format::RG32_FLOAT, .offset = offsetof(ImDrawVert, uv), .stride = sizeof(ImDrawVert) },
            { .name = "COLOR0", .format = Format::RGBA8_UNORM,   .offset = offsetof(ImDrawVert, col), .stride = sizeof(ImDrawVert) },
        },
    };
    mPipeline = CreateHandle<Pipeline>(device, pipelineDesc);
}

void GUI::CreateFontTexture()
{
    uint8_t* fontData;
    int textureWidth, textureHeight, bytesPerPixel;
    ImGuiIO& io = ImGui::GetIO();
    io.Fonts->GetTexDataAsRGBA32(&fontData, &textureWidth, &textureHeight, &bytesPerPixel);

    const uint64_t uploadSizeInBytes = textureWidth * textureHeight * bytesPerPixel;
    Buffer stagingBuffer = Buffer(mDevice, { .byteSize = uploadSizeInBytes, .access = MemoryAccess::HOST });
    memcpy(stagingBuffer.GetMappedData(), fontData, uploadSizeInBytes);

    Handle<CommandList> cmdList = mDevice.CreateCommandList();
    cmdList->Open();

    const TextureDesc texDesc = {
        .dimensions = { textureWidth, textureHeight, 1u },
        .format = Format::RGBA8_UNORM,
        .usage = TextureUsageBits::SAMPLED,
    };
    mFontTexture = CreateHandle<Texture>(mDevice, texDesc);

    cmdList->SetResourceState(*mFontTexture, ResourceStateBits::COPY_DEST);
    cmdList->WriteTexture(mFontTexture.get(), stagingBuffer);
    cmdList->SetResourceState(*mFontTexture, ResourceStateBits::SHADER_RESOURCE);

    cmdList->Close();
    mDevice.ExecuteCommandList(cmdList);
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

    ImGui::SliderFloat("Choppiness", &mGuiParams.choppiness, 0.f, 2.5f);
    ImGui::SliderInt("Sun Elevation", &mGuiParams.sunElevation, 0, 89);
    ImGui::SliderInt("Sun Azimuth", &mGuiParams.sunAzimuth, 0, 359);
    ImGui::SliderFloat("Displacement Scale Factor", &mGuiParams.displacementScaleFactor, 0, 100);

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
    if (vertexBuffer == nullptr || vertexBuffer->GetSizeInBytes() < vertexBufferSize) {
        const BufferDesc desc = {
            .byteSize = GetAlignedSize(vertexBufferSize, 32'000ull),
            .access = MemoryAccess::HOST,
            .usage = BufferUsageBits::VERTEX
        };
        vertexBuffer = CreateHandle<Buffer>(mDevice, desc);
    }
    if (indexBuffer == nullptr || indexBuffer->GetSizeInBytes() < indexBufferSize) {
        const BufferDesc desc = {
            .byteSize = GetAlignedSize(indexBufferSize, 16'000ull),
            .access = MemoryAccess::HOST,
            .usage = BufferUsageBits::INDEX
        };
        indexBuffer = CreateHandle<Buffer>(mDevice, desc);
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

void GUI::DrawFrame(Handle<CommandList> cmdList, const Texture& renderTarget, uint32_t frameIndex)
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

    ImDrawData* drawData = ImGui::GetDrawData();
    const int fbWidth  = drawData->DisplaySize.x * drawData->FramebufferScale.x;
    const int fbHeight = drawData->DisplaySize.y * drawData->FramebufferScale.y;
    if (fbWidth <= 0 || fbHeight <= 0) return; // Window is minimized
    // These clip variables are used to project scissor/clipping rectangles into framebuffer space
    const ImVec2 clipOffset = drawData->DisplayPos;       // (0,0) unless using multi-viewports
    const ImVec2 clipScale  = drawData->FramebufferScale; // (1,1) unless using retina display which are often (2,2)
    if (drawData->CmdListsCount <= 0) return;

    GraphicsState state = {
        .pipeline = mPipeline,
        .viewport = Viewport(fbWidth, fbHeight),
        .colorAttachments = {{ .texture = &renderTarget, .loadOp = LoadOp::LOAD }},
        .bindings = { Binding(*mFontTexture) },
        .vertexBuffer = vertexBuffer,
        .indexBuffer = indexBuffer,
        .pushConstants = { .byteSize = sizeof(PushConstantData), .data = (void*)&pushConstantData },
    };
    cmdList->SetGraphicsState(state);

    int globalIndexOffset = 0, globalVertexOffset = 0;
    for (int listIdx = 0; listIdx < drawData->CmdListsCount; ++listIdx) {
        const ImDrawList* imCmdList = drawData->CmdLists[listIdx];
        for (int bufIdx = 0; bufIdx < imCmdList->CmdBuffer.Size; ++bufIdx) {
            const ImDrawCmd* drawCmd = &imCmdList->CmdBuffer[bufIdx];

            // Project scissor/clipping rectangles into framebuffer space and
            // clamp to viewport as Vulkan doesn't accept scissor rects that are off bounds
            const Rect scissorRect = Rect(
                (int)std::max((drawCmd->ClipRect.x - clipOffset.x) * clipScale.x, 0.0f), // minX
                (int)std::min((drawCmd->ClipRect.z - clipOffset.x) * clipScale.x, float(fbWidth)), // maxX
                (int)std::max((drawCmd->ClipRect.y - clipOffset.y) * clipScale.y, 0.0f), // minY
                (int)std::min((drawCmd->ClipRect.w - clipOffset.y) * clipScale.y, float(fbHeight)) // maxY
            );
            if (scissorRect.maxX <= scissorRect.minX || scissorRect.maxY <= scissorRect.minY) continue;
            state.viewport.scissorRect = scissorRect;

            cmdList->DrawIndexed({
                .vertexCount = drawCmd->ElemCount,
                .startVertexLocation = drawCmd->VtxOffset + globalVertexOffset,
                .startIndexLocation = drawCmd->IdxOffset + globalIndexOffset,
            });
        }
        globalIndexOffset += imCmdList->IdxBuffer.Size;
        globalVertexOffset += imCmdList->VtxBuffer.Size;
    }
}