#pragma on

#include "fluid/structs.h"

class Device;
class Window;
class Texture;
class Buffer;
class Pipeline;
class Texture;
class Swapchain;
class CommandList;


class GUI {
public:
    GUI(const Device& device, const Swapchain& swapchain, const Window& window);
    ~GUI();

    void NewFrame();
    void DrawFrame(Handle<CommandList> cmdList, const Texture& renderTarget, uint32_t frameIndex);
    FluidSimParams GetParams() const { return mGuiParams; }
    void SetParams(const FluidSimParams& params) { mGuiParams = params; mHasParamsChanged = true; }

    void SetDebugImage(Handle<Texture> image) { mDebugImage = image; }

private:
    void CreateFontTexture();
    void UpdateBuffers(uint32_t frameIndex);

    Handle<Pipeline> mPipeline;
    Handle<Texture> mFontTexture;
    std::array<Handle<Buffer>, 2> mVertexBuffers;
    std::array<Handle<Buffer>, 2> mIndexBuffers;
    FluidSimParams mGuiParams;
    bool mHasParamsChanged = false;
    Handle<Texture> mDebugImage = nullptr;

    const Device& mDevice;
    const Window& mWindow;
    const Swapchain& mSwapchain;
};