// lib/src/renderer_impl.h
#pragma once

#include "terrain/terrain.h"
#include <string>

namespace terrain {

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
    
    // 临时存储配置
    Config m_config;
    RenderTarget m_currentTarget;
    
    // 后续会添加更多成员
};

} // namespace terrain