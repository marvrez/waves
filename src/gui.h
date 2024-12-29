#pragma on

class Device;
class Window;
class Texture;
class Buffer;
class Pipeline;
class Texture;
class Swapchain;
class CommandList;

enum class DisplayMode : int {
    DENSITY = 0,
    VELOCITY = 1,
    TILE_TAG = 2,
    DIVERGENCE = 3,
    JACOBI = 4,
    RESIDUAL = 5,
    GRADIENT = 6,
};

struct GUIParams {
    float slice = 0.5f;
    int currentFrame = 0;
    int targetFrame = 1;
    DisplayMode displayMode = DisplayMode::DENSITY;
    int currentLevel = 0;
    bool shouldShowGrid = false;
    bool shouldShowTileAllocation = false;
};

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
    GUIParams mGuiParams;
    bool mHasParamsChanged = false;

    const Device& mDevice;
    const Window& mWindow;
    const Swapchain& mSwapchain;
};