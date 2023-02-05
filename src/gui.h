#pragma on

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

private:
    void CreateFontTexture();
    void UpdateBuffers(uint32_t frameIndex);

    Handle<Pipeline> mPipeline;
    Handle<Texture> mFontTexture;
    std::array<Handle<Buffer>, 2> mVertexBuffers;
    std::array<Handle<Buffer>, 2> mIndexBuffers;

    const Device& mDevice;
    const Window& mWindow;
    const Swapchain& mSwapchain;
};