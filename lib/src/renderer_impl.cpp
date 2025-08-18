// lib/src/renderer_impl.cpp
#include "renderer_impl.h"
#include <cstdio>

namespace terrain {

RendererImpl::RendererImpl() {
}

RendererImpl::~RendererImpl() {
    if (m_initialized) {
        Shutdown();
    }
}

bool RendererImpl::Initialize(const Config* config) {
    if (m_initialized) {
        m_lastError = "Already initialized";
        return false;
    }
    
    if (config) {
        m_config = *config;
    }
    
    // TODO: 初始化bgfx等
    printf("[TerrainRenderer] Initializing with size %dx%d\n", 
           m_config.windowWidth, m_config.windowHeight);
    
    m_initialized = true;
    return true;
}

void RendererImpl::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    printf("[TerrainRenderer] Shutting down\n");
    
    // TODO: 清理资源
    
    m_initialized = false;
}

void RendererImpl::SetRenderTarget(const RenderTarget& target) {
    m_currentTarget = target;
    printf("[TerrainRenderer] Set render target: %dx%d\n", target.width, target.height);
}

bool RendererImpl::LoadHeightmap(const std::string& path) {
    printf("[TerrainRenderer] Loading heightmap: %s\n", path.c_str());
    // TODO: 实现加载
    return true;
}

bool RendererImpl::LoadTexture(const std::string& path) {
    printf("[TerrainRenderer] Loading texture: %s\n", path.c_str());
    // TODO: 实现加载
    return true;
}

void RendererImpl::BeginFrame() {
    // TODO: 实现
}

void RendererImpl::SetViewProjection(const float* view, const float* proj) {
    // TODO: 实现
}

void RendererImpl::Draw() {
    // TODO: 实现
}

void RendererImpl::EndFrame() {
    // TODO: 实现
}

void RendererImpl::SetWireframe(bool enable) {
    printf("[TerrainRenderer] Wireframe: %s\n", enable ? "ON" : "OFF");
}

void RendererImpl::SetScale(float horizontalScale, float verticalScale) {
    printf("[TerrainRenderer] Scale: %.2f x %.2f\n", horizontalScale, verticalScale);
}

const char* RendererImpl::GetLastError() const {
    return m_lastError.c_str();
}

} // namespace terrain