#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

constexpr glm::vec3 UP_DIRECTION = glm::vec3(0.f, 1.f, 0.f);

class Window;
struct Camera
{
    Camera(
        glm::vec3 position,
        float near = 0.1f,
        float far = 1000.f,
        float movementSpeed = 10000.f,
        float mouseSensitivity = 0.1f
    );

    inline glm::mat4 GetViewProjectionMatrix(float aspectRatio) const
    {
        return glm::perspective(glm::radians(mFovY), aspectRatio, mNear, mFar) * glm::lookAt(mPosition, mPosition + mFront, mUp);
    }

    inline glm::vec3 GetPosition() const { return mPosition; }

    void ProcessKeyboard(const Window& window, float dt);
    void ProcessMouseMove(float dx, float dy);
    void ProcessMouseScroll(float dScroll);

private:
    void UpdateCameraVectors();

    glm::vec3 mPosition;

    glm::vec3 mFront; // Z
    glm::vec3 mRight; // X
    glm::vec3 mUp;    // Y

    float mMovementSpeed;
    float mMouseSensitivity;

    float mPitch = 0.f; // X
    float mYaw = -90.f; // Y
    float mFovY = 60.f;

    float mNear, mFar;
};