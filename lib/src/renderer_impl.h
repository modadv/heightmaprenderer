// lib/src/renderer_impl.h
#pragma once

#include "terrain/terrain.h"
#include <string>
#include <memory>
#include <bgfx/bgfx.h>

namespace terrain {

namespace internal {
    class RenderContext;
    class HeightmapData;  // 前向声明
}

class RendererImpl {
public:
    RendererImpl();
    ~RendererImpl();
    
    bool Initialize(const Config* config);
    void Shutdown();
    
    void SetRenderTarget(const RenderTarget& target);
    bool LoadHeightmap(const std::string& path);
    bool LoadTexture(const std::string& path);
    
    void BeginFrame();
    void SetViewProjection(const float* view, const float* proj);
    void Draw();
    void EndFrame();
    
    void SetWireframe(bool enable);
    void SetScale(float horizontalScale, float verticalScale);
    
    const char* GetLastError() const;
    
private:
    bool m_initialized = false;
    std::string m_lastError;
    
    // 渲染上下文
    std::unique_ptr<internal::RenderContext> m_context;
    
    // 高度图数据
    std::unique_ptr<internal::HeightmapData> m_heightmap;
    
    // 纹理句柄
    bgfx::TextureHandle m_heightmapTexture = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_slopeMapTexture = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_diffuseTexture = BGFX_INVALID_HANDLE;
    
    // 配置
    Config m_config;
    RenderTarget m_currentTarget;
    
    // 渲染状态
    bool m_wireframe = false;
    float m_scaleH = 1.0f;
    float m_scaleV = 1.0f;
    
    // 矩阵（4x4）
    float m_viewMatrix[16];
    float m_projMatrix[16];
    
    // 创建纹理
    bool CreateHeightmapTexture();
    bool CreateSlopeMapTexture();
};

} // namespace terrain