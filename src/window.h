#pragma once

#include "logger.h"

#include <GLFW/glfw3.h>

class Window {
public:
    Window(int width, int height, const char* title, bool resizable=true)
        : mWidth(width), mHeight(height)
    {
        if (glfwInit() != GLFW_TRUE) LOG_ERROR("Could not init GLFW.");
        if (glfwVulkanSupported() != GLFW_TRUE) LOG_ERROR("Could not initialize Vulkan for GLFW");
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, resizable ? GLFW_TRUE : GLFW_FALSE);

        mWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
        if (!mWindow) LOG_ERROR("Could not create GLFW window.");
        glfwMakeContextCurrent(mWindow);
        glfwSwapInterval(0);
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

private:
    int mWidth, mHeight;
    GLFWwindow* mWindow;
};