#pragma once

#include <GLFW/glfw3.h>

#include "logger.h"

constexpr int MAX_KEYS = 512, MAX_MOUSE_KEYS = 8;

enum Keycode {
    KEY_SPACE = 32, KEY_APOSTROPHE = 39, /* ' */ KEY_COMMA = 44, /* , */ KEY_MINUS = 45, /* - */ KEY_PERIOD = 46, /* . */ KEY_SLASH = 47, /* / */
    KEY_0 = 48, KEY_1 = 49, KEY_2 = 50, KEY_3 = 51, KEY_4 = 52, KEY_5 = 53, KEY_6 = 54, KEY_7 = 55, KEY_8 = 56, KEY_9 = 57,
    KEY_SEMICOLON = 59, /* ; */ KEY_EQUAL = 61,     /* = */
    KEY_A = 65, KEY_B = 66, KEY_C = 67, KEY_D = 68, KEY_E = 69, KEY_F = 70, KEY_G = 71, KEY_H = 72, KEY_I = 73, KEY_J = 74,
    KEY_K = 75, KEY_L = 76, KEY_M = 77, KEY_N = 78, KEY_O = 79, KEY_P = 80, KEY_Q = 81, KEY_R = 82, KEY_S = 83, KEY_T = 84,
    KEY_U = 85, KEY_V = 86, KEY_W = 87, KEY_X = 88, KEY_Y = 89, KEY_Z = 90, KEY_LEFT_BRACKET = 91,  /* [ */
    KEY_BACKSLASH = 92, /* \ */ KEY_RIGHT_BRACKET = 93, /* ] */ KEY_GRAVE_ACCENT = 96, /* ` */ KEY_WORLD_1 = 161, /* non-US #1 */ KEY_WORLD_2 = 162, /* non-US #2 */
    KEY_ESCAPE = 256, KEY_ENTER = 257, KEY_TAB = 258, KEY_BACKSPACE = 259, KEY_INSERT = 260, KEY_DELETE = 261,
    KEY_RIGHT = 262, KEY_LEFT = 263, KEY_DOWN = 264, KEY_UP = 265,
    KEY_PAGE_UP = 266, KEY_PAGE_DOWN = 267, KEY_HOME = 268, KEY_END = 269, KEY_CAPS_LOCK = 280, KEY_SCROLL_LOCK = 281, KEY_NUM_LOCK = 282, KEY_PRINT_SCREEN = 283, KEY_PAUSE = 284,
    KEY_F1 = 290, KEY_F2 = 291, KEY_F3 = 292, KEY_F4 = 293, KEY_F5 = 294, KEY_F6 = 295, KEY_F7 = 296, KEY_F8 = 297, KEY_F9 = 298,
    KEY_F10 = 299, KEY_F11 = 300, KEY_F12 = 301, KEY_F13 = 302, KEY_F14 = 303, KEY_F15 = 304, KEY_F16 = 305, KEY_F17 = 306,
    KEY_F18 = 307, KEY_F19 = 308, KEY_F20 = 309, KEY_F21 = 310, KEY_F22 = 311, KEY_F23 = 312, KEY_F24 = 313, KEY_F25 = 314,
    KEY_KP_0 = 320, KEY_KP_1 = 321, KEY_KP_2 = 322, KEY_KP_3 = 323, KEY_KP_4 = 324, KEY_KP_5 = 325, KEY_KP_6 = 326, KEY_KP_7 = 327, KEY_KP_8 = 328, KEY_KP_9 = 329,
    KEY_KP_DECIMAL = 330, KEY_KP_DIVIDE = 331, KEY_KP_MULTIPLY = 332, KEY_KP_SUBTRACT = 333, KEY_KP_ADD = 334, KEY_KP_ENTER = 335, KEY_KP_EQUAL = 336,
    KEY_LEFT_SHIFT = 340, KEY_LEFT_CONTROL = 341, KEY_LEFT_ALT = 342, KEY_LEFT_SUPER = 343,
    KEY_RIGHT_SHIFT = 344, KEY_RIGHT_CONTROL = 345, KEY_RIGHT_ALT = 346, KEY_RIGHT_SUPER = 347,
    KEY_MENU = 348
};

enum Mousekey {
    BUTTON_1 = 0, BUTTON_2 = 1, BUTTON_3 = 2, BUTTON_4 = 3, BUTTON_5 = 4, BUTTON_6 = 5, BUTTON_7 = 6, BUTTON_8 = 7,
    BUTTON_LAST = 7, BUTTON_LEFT = 0, BUTTON_RIGHT = 1, BUTTON_MIDDLE = 2
};

class Window {
public:
    Window(int width, int height, const char* title, bool resizable=true)
        : mWidth(width), mHeight(height)
    {
        glfwSetErrorCallback([](int code, const char* message) {
            LOG_ERROR("GLFW error: {0:#x} - {1:}", code, message);
        });
        if (glfwInit() != GLFW_TRUE) LOG_ERROR("Could not init GLFW.");
        if (glfwVulkanSupported() != GLFW_TRUE) LOG_ERROR("Could not initialize Vulkan for GLFW");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);

        mWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!mWindow) LOG_ERROR("Could not create GLFW window.");

        glfwSetWindowUserPointer(mWindow, this);
        glfwSetKeyCallback(mWindow, Window::KeyCallback);
        glfwSetMouseButtonCallback(mWindow, Window::MouseButtonCallback);
    }

    ~Window()
    {
        LOG_INFO("Destroying window...");
        if (mWindow) {
            glfwDestroyWindow(mWindow);
            glfwTerminate();
            mWindow = nullptr;
        }
    }

    void Close() { glfwSetWindowShouldClose(mWindow, GLFW_TRUE); }
    bool ShouldClose() { return glfwWindowShouldClose(mWindow); }
    void PollEvents()
    {
        glfwPollEvents();
    }
    GLFWwindow* GetHandle() const { return mWindow; }

    std::pair<uint32_t, uint32_t> GetFramebufferSize() const
    {
        int width, height;
        glfwGetFramebufferSize(mWindow, &width, &height);
        return { (uint32_t)width, (uint32_t)height };
    }

    std::pair<uint32_t, uint32_t> GetWindowSize() const
    {
        int width, height;
        glfwGetWindowSize(mWindow, &width, &height);
        return { (uint32_t)width, (uint32_t)height };
    }

    std::pair<double, double> GetCursorPosition() const
    {
        double xMousePosition, yMousePosition;
        glfwGetCursorPos(mWindow, &xMousePosition, &yMousePosition);
        return { xMousePosition, yMousePosition };
    }

    bool IsMouseLeftButtonPressed() const { return IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT); }
    bool IsMouseRightButtonPressed() const { return IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT); }

    bool Pressed(Keycode key) const
    {
        const int k = static_cast<int>(key);
        if (k <= 0 || k >= (int)mKeys.size()) return false;
        return mKeys[k];
    }
    bool MousePressed(Mousekey key) const
    {
        const int k = static_cast<int>(key);
        if (k <= 0 || k >= (int)mMouseKeys.size()) return false;
        return mMouseKeys[k];
    }

private:
    bool IsMouseButtonPressed(int button) const { return glfwGetMouseButton(mWindow, button) == GLFW_PRESS; }
    int mWidth, mHeight;
    GLFWwindow* mWindow;

    std::bitset<MAX_KEYS> mKeys;
    std::bitset<MAX_MOUSE_KEYS> mMouseKeys;
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int)
    {
        auto win = (Window*)glfwGetWindowUserPointer(window);
        if ((unsigned int)button >= win->mMouseKeys.size()) return;
        if (action == GLFW_PRESS) win->mMouseKeys[button] = true;
        else if (action == GLFW_RELEASE) win->mMouseKeys[button] = false;
    }
    static void KeyCallback(GLFWwindow* window, int key, int, int action, int)
    {
        auto win = (Window*)glfwGetWindowUserPointer(window);
        if ((unsigned int)key >= win->mKeys.size()) return;
        if (action == GLFW_PRESS) win->mKeys[key] = true;
        else if (action == GLFW_RELEASE) win->mKeys[key] = false;
    }
};