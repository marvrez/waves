#pragma once

#include "vk/pipeline.h"

// TODO: We probably don't need this struct; texture coordinates can be computed on the fly, right?
struct GridVertex {
    glm::vec3 pos;
    glm::vec2 uv;
};

struct Grid {
    std::vector<GridVertex> vertices;
    std::vector<uint32_t> indices;
};

struct GridMesh {
    Handle<Buffer> vertexBuffer;
    Handle<Buffer> indexBuffer;
};

Grid MakeGrid(int32_t gridSize);
GridMesh MakeGridMesh(const Device& device, const Grid& grid);