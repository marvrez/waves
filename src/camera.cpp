#include "camera.h"

#include <GLFW/glfw3.h>
#include "window.h"
#include <iostream>

Camera::Camera(glm::vec3 position, float near, float far, float movementSpeed, float mouseSensitivity)
    : mPosition(position), mMovementSpeed(movementSpeed), mMouseSensitivity(mouseSensitivity), mNear(near), mFar(far)
{
    UpdateCameraVectors();
}

void Camera::ProcessKeyboard(const Window& window, float dt)
{
    const float movedDistance = mMovementSpeed * dt;
    if (window.Pressed(Keycode::KEY_W)) mPosition += mFront * movedDistance * 0.1f;
    if (window.Pressed(Keycode::KEY_S)) mPosition -= mFront * movedDistance * 0.1f;
    if (window.Pressed(Keycode::KEY_A)) mPosition -= mRight * movedDistance * 0.1f;
    if (window.Pressed(Keycode::KEY_D)) mPosition += mRight * movedDistance * 0.1f;
    if (window.Pressed(Keycode::KEY_E)) mPosition += mUp * movedDistance;
    if (window.Pressed(Keycode::KEY_Q)) mPosition -= mUp * movedDistance;
    if (window.Pressed(Keycode::KEY_LEFT)) mYaw -= movedDistance;
    if (window.Pressed(Keycode::KEY_RIGHT)) mYaw += movedDistance;
    if (window.Pressed(Keycode::KEY_UP)) mPitch += movedDistance;
    if (window.Pressed(Keycode::KEY_DOWN)) mPitch -= movedDistance;
    UpdateCameraVectors();
}

void Camera::ProcessMouseMove(float dx, float dy)
{
    mYaw += mMouseSensitivity * dx;
    mPitch += mMouseSensitivity * dy;
    mPitch = glm::clamp(mPitch, -89.f, 89.f);
    UpdateCameraVectors();
}

void Camera::ProcessMouseScroll(float dScroll)
{
    mFovY -= dScroll;
    mFovY = glm::clamp(mFovY, 1.f, 45.f);
}

void Camera::UpdateCameraVectors()
{
    mFront.x = glm::cos(glm::radians(mYaw)) * glm::cos(glm::radians(mPitch));
    mFront.y = glm::sin(glm::radians(mPitch));
    mFront.z = glm::sin(glm::radians(mYaw)) * glm::cos(glm::radians(mPitch));
    mFront = glm::normalize(mFront);

    mRight = glm::normalize(glm::cross(mFront, UP_DIRECTION));
    mUp = glm::normalize(glm::cross(mRight, mFront));
}