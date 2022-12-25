#include "window.h"

const int WINDOW_WIDTH = 1280;
const int WINDOW_HEIGHT = 720;

int main()
{

    Window window = Window(WINDOW_WIDTH, WINDOW_HEIGHT, "sdf-edit");

    while (!window.ShouldClose()) {
        window.PollEvents();
    }

    return 0;
}