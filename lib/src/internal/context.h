// lib/src/internal/context.h
#pragma once

#include <bgfx/bgfx.h>
#include <memory>
#include "terrain/terrain.h"

namespace terrain {
namespace internal {

class RenderContext {
public:
    RenderContext();
    ~RenderContext();
    
    bool Initialize(uint32_t width, uint32_t height, void* nativeHandle = nullptr);
    void Shutdown();
    
    void SetRenderTarget(const RenderTarget& target);
    void BeginFrame();
    void EndFrame();
    
    // 获取当前视图尺寸
    uint32_t GetWidth() const { return m_width; }
    uint32_t GetHeight() const { return m_height; }
    
    // 检查是否初始化
    bool IsInitialized() const { return m_initialized; }
    
private:
    bool m_initialized = false;
    uint32_t m_width = 0;
    uint32_t m_height = 0;
    void* m_nativeHandle = nullptr;
    
    // 视图ID
    bgfx::ViewId m_viewId = 0;
};

} // namespace internal
} // namespace terrain