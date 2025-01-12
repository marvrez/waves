#include "vk/descs.h"
#include "vk/device.h"
#include "vk/texture.h"

#include <vector>

class TexturePool {
public:
    TexturePool(const Device& device, TextureDesc textureDesc)
        : mDevice(device), mTextureDesc(textureDesc) {}

    // Acquire a texture from the pool or create a new one if none are available
    Handle<Texture> Acquire() {
        if (mPool.empty()) return CreateHandle<Texture>(mDevice, mTextureDesc);
        auto texture = mPool.back();
        mPool.pop_back();
        return texture;
    }

    // Release a texture back to the pool for reuse
    void Release(Handle<Texture> texture) {
        mPool.push_back(texture);
    }

    // Clear all resources in the pool (if necessary, e.g., on shutdown)
    void Clear() {
        mPool.clear();
    }

private:
    const Device& mDevice;
    TextureDesc mTextureDesc;
    std::vector<Handle<Texture>> mPool;
};
