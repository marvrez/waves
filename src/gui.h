#pragma on

class Device;
class Window;
class Texture;
class Buffer;
class Pipeline;
class Texture;
class Swapchain;

class GUI {
public:
    GUI(const Device& device, const Swapchain& swapchain, const Window& window);
    ~GUI();

    void NewFrame();
    void DrawFrame(VkCommandBuffer cmdBuf, uint32_t swapchainImageIndex, uint32_t frameIndex);

private:
    void CreateFontTexture();
    void UpdateBuffers(uint32_t frameIndex);

    std::unique_ptr<Pipeline> mPipeline;
    std::unique_ptr<Texture> mFontTexture;
    std::array<std::unique_ptr<Buffer>, 2> mVertexBuffers;
    std::array<std::unique_ptr<Buffer>, 2> mIndexBuffers;

    const Device& mDevice;
    const Window& mWindow;
    const Swapchain& mSwapchain;
};