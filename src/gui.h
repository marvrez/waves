#pragma on

class Device;
class Window;
class Texture;
class Buffer;
class Pipeline;
class Texture;
class Swapchain;
class CommandList;

struct GUIParams {
    float choppiness;
    float displacementScaleFactor = 1.0f;

    float windMagnitude = 14.142135f;
    float windAngle = 45.f;

    // In degrees
    int sunElevation;
    int sunAzimuth;
};

class GUI {
public:
    GUI(const Device& device, const Swapchain& swapchain, const Window& window);
    ~GUI();

    void NewFrame();
    GUIParams GetParams() const { return mGuiParams; }
    void DrawFrame(Handle<CommandList> cmdList, const Texture& renderTarget, uint32_t frameIndex);

private:
    void CreateFontTexture();
    void UpdateBuffers(uint32_t frameIndex);

    Handle<Pipeline> mPipeline;
    Handle<Texture> mFontTexture;
    std::array<Handle<Buffer>, 2> mVertexBuffers;
    std::array<Handle<Buffer>, 2> mIndexBuffers;
    GUIParams mGuiParams = { .choppiness = 1.5f, .sunElevation = 0, .sunAzimuth = 90 };

    const Device& mDevice;
    const Window& mWindow;
    const Swapchain& mSwapchain;
};