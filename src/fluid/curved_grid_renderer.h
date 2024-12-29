#pragma once

class Device;
class Buffer;
class Pipeline;
class Swapchain;
class CommandList;
class Texture;
struct Camera;

struct CurvedGridMesh {
    Handle<Buffer> vertexBuffer;
    Handle<Buffer> indexBuffer;
};

class CurvedGridRenderer {
public:
    CurvedGridRenderer(const Device& device, const Swapchain& swapchain, const Camera& camera);
    void Render(Handle<CommandList> cmdList, const Texture& renderTarget);

private:
    const Device& mDevice;
    const Swapchain& mSwapchain;
    const Camera& mCamera;
    CurvedGridMesh mGrid;
    Handle<Pipeline> mPipeline;
};