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
    float displacementScaleFactor = 8.0f;
    float tipScaleFactor = 5.0f;
    float exposure = 0.35f;

    float windMagnitude = 14.142135f;
    float windAngle = 45.f;

    // In degrees
    int sunElevation;
    int sunAzimuth;

    bool isInWireframeMode = false;
};

class GUI {
public:
    GUI(const Device& device, const Swapchain& swapchain, const Window& window);
    ~GUI();

    void NewFrame();
    GUIParams GetParams() const { return mGuiParams; }
    bool hasWindParamsChanged() const { return mHasWindParamsChanged; }
    void DrawFrame(Handle<CommandList> cmdList, const Texture& renderTarget, uint32_t frameIndex);

private:
    void CreateFontTexture();
    void UpdateBuffers(uint32_t frameIndex);

    Handle<Pipeline> mPipeline;
    Handle<Texture> mFontTexture;
    std::array<Handle<Buffer>, 2> mVertexBuffers;
    std::array<Handle<Buffer>, 2> mIndexBuffers;
    GUIParams mGuiParams = { .choppiness = 1.5f, .sunElevation = 0, .sunAzimuth = 90, .windMagnitude = 14.142135f, .windAngle = 45.f };
    bool mHasWindParamsChanged = false;

    const Device& mDevice;
    const Window& mWindow;
    const Swapchain& mSwapchain;
};