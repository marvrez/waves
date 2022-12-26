#include "window.h"

#include "vk/device.h"
#include "vk/buffer.h"

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
    while (!window.ShouldClose()) {
        window.PollEvents();
    }

    return 0;
}