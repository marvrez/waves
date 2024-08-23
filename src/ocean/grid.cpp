
#include <cstdlib>
#include <vector>

#include <glm/glm.hpp>

#include "vk/device.h"
#include "vk/buffer.h"

#include "grid.h"
#include <cstddef>
#include <iostream>

constexpr float kUvScale = 2.0f;

Grid MakeGrid(int32_t gridSize)
{
    const int vertexCount = gridSize + 1;
    std::vector<GridVertex> vertices(vertexCount * vertexCount);
    // 2 triangles per quad and 3 vertices per triangle
    std::vector<uint32_t> indices(gridSize * gridSize * 2 * 3);

    int currentIdx = 0;
    for (int z = -gridSize / 2; z <= gridSize / 2; ++z) {
        for (int x = -gridSize / 2; x <= gridSize / 2; ++x) {
            vertices[currentIdx].pos = glm::vec3(float(x), 0.0f, float(z));

            const float u = (float(x) / gridSize) + 0.5f;
            const float v = (float(z) / gridSize) + 0.5f;
            vertices[currentIdx++].uv = glm::vec2(u, v) * kUvScale;
        }
    }
    assert(currentIdx == vertices.size());

    // Generate triangle indices for the grid – from the bottom-left corner
    // NOTE: Clockwise winding of triangle
    currentIdx = 0;
    for (int y = 0; y < gridSize; ++y) {
        for (int x = 0; x < gridSize; ++x) {
            indices[currentIdx++] = (vertexCount * y) + x;
            indices[currentIdx++] = (vertexCount * (y + 1)) + x;
            indices[currentIdx++] = (vertexCount * y) + x + 1;
    
            indices[currentIdx++] = (vertexCount * y) + x + 1;
            indices[currentIdx++] = (vertexCount * (y + 1)) + x;
            indices[currentIdx++] = (vertexCount * (y + 1)) + x + 1;
        }
    }
    assert(currentIdx == indices.size());
    return { .vertices = vertices, .indices = indices };
}


GridMesh MakeGridMesh(const Device& device, const Grid& grid)
{
    auto vertexBuffer = CreateHandle<Buffer>(device, BufferDesc{
        .byteSize = grid.vertices.size() * sizeof(GridVertex),
        .access = MemoryAccess::HOST, // TODO: Should this be DEVICE?
        .usage = BufferUsageBits::VERTEX,
        .data = grid.vertices.data()
    });
    auto indexBuffer = CreateHandle<Buffer>(device, BufferDesc{
        .byteSize = grid.indices.size() * sizeof(uint32_t),
        .access = MemoryAccess::HOST, // TODO: Should this be DEVICE?
        .usage = BufferUsageBits::INDEX,
        .data = grid.indices.data()
    });
    return { .vertexBuffer = vertexBuffer, .indexBuffer = indexBuffer };
}