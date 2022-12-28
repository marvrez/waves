#include "window.h"

#include "vk/device.h"
#include "vk/buffer.h"
#include "vk/texture.h"
#include "vk/frame_pacing.h"

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

int main()
{
    Window window = Window(WINDOW_WIDTH, WINDOW_HEIGHT, "sdf-edit");

    const char* data = "AHHHH";
    Device device = Device(window, true);
    const BufferDesc desc = {
        .byteSize = 1024,
        .access = MemoryAccess::DEVICE,
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        .data = (void*)data
    };
    Buffer deviceBuffer = Buffer(device, desc);

    TextureDesc texDesc = {
        .width = 100,
        .height = 100,
        .format = VK_FORMAT_R8G8B8A8_UNORM,
        .usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
    };
    Texture tex = Texture(device, texDesc);

    FramePacingState framePacingState = FramePacingState(device);

    while (!window.ShouldClose()) {
        window.PollEvents();
    }

    return 0;
}