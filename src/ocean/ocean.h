#pragma once

struct OceanPushConstantData {
    glm::mat4 worldToClip;

    glm::vec3 cameraPosition;
    float displacementScaleFactor;

    glm::vec3 sunDirection;
    float tipScaleFactor;

    float exposure;
};

struct InitialSpectrumPushConstantData {
    glm::vec2 windDirection;
    int texSize;
    int oceanSize;
};

struct PhasePushConstantData {
    float dt;
    int texSize;
    int oceanSize;
};

struct SpectrumPushConstantData {
    int texSize;
    int oceanSize;
    float choppiness;
};

struct NormalMapPushConstantData {
    int texSize;
    int oceanSize;
};

struct FFTPushConstantData {
    int totalCount;
    int subseqCount;
};