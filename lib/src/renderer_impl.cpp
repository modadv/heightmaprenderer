// lib/src/renderer_impl.cpp
#include "renderer_impl.h"
#include "internal/context.h"
#include "internal/heightmap_loader.h"
#include "internal/simple_terrain.h"
#include <bgfx/bgfx.h>
#include <bx/math.h>
#include <cstdio>
#include <cstring>
#include <vector>

namespace terrain {


RendererImpl::RendererImpl() 
    : m_context(std::make_unique<internal::RenderContext>())
    , m_heightmap(std::make_unique<internal::HeightmapData>())
    , m_terrain(std::make_unique<internal::SimpleTerrain>()) {
    // 初始化单位矩阵
    bx::mtxIdentity(m_viewMatrix);
    bx::mtxIdentity(m_projMatrix);
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
    
    // 初始化渲染上下文
    if (!m_context->Initialize(m_config.windowWidth, m_config.windowHeight)) {
        m_lastError = "Failed to initialize render context";
        return false;
    }
    
    // 初始化地形渲染器
    if (!m_terrain->Initialize()) {
        m_lastError = "Failed to initialize terrain renderer";
        return false;
    }
    
    // 设置默认地形缩放
    m_terrain->SetTerrainScale(m_scaleH, m_scaleV, m_scaleH);
    
    // 设置调试模式
    if (m_config.debug) {
        bgfx::setDebug(BGFX_DEBUG_TEXT);
    }
    
    printf("[RendererImpl] Initialized successfully\n");
    
    m_initialized = true;
    return true;
}

void RendererImpl::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    printf("[RendererImpl] Shutting down\n");
    
    // 关闭地形渲染器
    m_terrain->Shutdown();
    
    // 清理纹理资源
    if (bgfx::isValid(m_heightmapTexture)) {
        bgfx::destroy(m_heightmapTexture);
        m_heightmapTexture = BGFX_INVALID_HANDLE;
    }
    
    if (bgfx::isValid(m_slopeMapTexture)) {
        bgfx::destroy(m_slopeMapTexture);
        m_slopeMapTexture = BGFX_INVALID_HANDLE;
    }
    
    if (bgfx::isValid(m_diffuseTexture)) {
        bgfx::destroy(m_diffuseTexture);
        m_diffuseTexture = BGFX_INVALID_HANDLE;
    }
    
    // 关闭渲染上下文
    m_context->Shutdown();
    
    m_initialized = false;
}

bool RendererImpl::LoadHeightmap(const std::string& path) {
    if (!m_initialized) {
        m_lastError = "Not initialized";
        return false;
    }
    
    printf("[RendererImpl] Loading heightmap: %s\n", path.c_str());
    
    // 加载高度图数据
    if (!m_heightmap->LoadFromFile(path)) {
        m_lastError = "Failed to load heightmap: " + m_heightmap->GetLastError();
        return false;
    }
    
    // 创建GPU纹理
    if (!CreateHeightmapTexture()) {
        m_lastError = "Failed to create heightmap texture";
        return false;
    }
    
    // 创建斜率图
    if (!CreateSlopeMapTexture()) {
        m_lastError = "Failed to create slope map texture";
        return false;
    }
    
    // 更新地形渲染器的纹理
    m_terrain->SetHeightmapTexture(m_heightmapTexture);
    m_terrain->SetSlopeMapTexture(m_slopeMapTexture);
    
    // 根据高度图的宽高比调整地形缩放
    float aspectRatio = m_heightmap->GetAspectRatio();
    m_terrain->SetTerrainScale(m_scaleH * aspectRatio, m_scaleV, m_scaleH);
    
    printf("[RendererImpl] Heightmap loaded successfully: %dx%d, aspect ratio: %.2f\n",
           m_heightmap->GetWidth(), m_heightmap->GetHeight(), 
           m_heightmap->GetAspectRatio());
    
    return true;
}

bool RendererImpl::CreateHeightmapTexture() {
    // 销毁旧纹理
    if (bgfx::isValid(m_heightmapTexture)) {
        bgfx::destroy(m_heightmapTexture);
    }
    
    int width = m_heightmap->GetWidth();
    int height = m_heightmap->GetHeight();
    
    if (width == 0 || height == 0) {
        return false;
    }
    
    // 创建纹理（使用原始16位数据）
    const bgfx::Memory* mem = bgfx::makeRef(
        m_heightmap->GetRawData(),
        width * height * sizeof(uint16_t)
    );
    
    m_heightmapTexture = bgfx::createTexture2D(
        uint16_t(width),
        uint16_t(height),
        false,  // no mips
        1,      // layers
        bgfx::TextureFormat::R16,
        BGFX_TEXTURE_NONE,
        mem
    );
    
    return bgfx::isValid(m_heightmapTexture);
}

bool RendererImpl::CreateSlopeMapTexture() {
    // 销毁旧纹理
    if (bgfx::isValid(m_slopeMapTexture)) {
        bgfx::destroy(m_slopeMapTexture);
    }
    
    // 生成斜率图
    std::vector<float> slopeMap = m_heightmap->GenerateSlopeMap();
    if (slopeMap.empty()) {
        return false;
    }
    
    int width = m_heightmap->GetWidth();
    int height = m_heightmap->GetHeight();
    
    // 创建纹理
    const bgfx::Memory* mem = bgfx::copy(
        slopeMap.data(),
        width * height * 2 * sizeof(float)
    );
    
    m_slopeMapTexture = bgfx::createTexture2D(
        uint16_t(width),
        uint16_t(height),
        false,  // no mips
        1,      // layers
        bgfx::TextureFormat::RG32F,
        BGFX_TEXTURE_NONE,
        mem
    );
    
    return bgfx::isValid(m_slopeMapTexture);
}

void RendererImpl::SetRenderTarget(const RenderTarget& target) {
    if (!m_initialized) {
        return;
    }
    
    m_currentTarget = target;
    m_context->SetRenderTarget(target);
}

bool RendererImpl::LoadTexture(const std::string& path) {
    if (!m_initialized) {
        m_lastError = "Not initialized";
        return false;
    }
    
    printf("[RendererImpl] Loading texture: %s\n", path.c_str());
    // TODO: 实现加载
    return true;
}

void RendererImpl::BeginFrame() {
    if (!m_initialized) {
        return;
    }
    
    m_context->BeginFrame();
}

void RendererImpl::SetViewProjection(const float* view, const float* proj) {
    if (!m_initialized) {
        return;
    }
    
    if (view) {
        memcpy(m_viewMatrix, view, sizeof(float) * 16);
    }
    if (proj) {
        memcpy(m_projMatrix, proj, sizeof(float) * 16);
    }
    
    // 设置变换矩阵
    bgfx::setViewTransform(0, m_viewMatrix, m_projMatrix);
}

void RendererImpl::Draw() {
    if (!m_initialized) {
        return;
    }
    
    // 使用SimpleTerrain进行渲染
    m_terrain->Draw(m_viewMatrix, m_projMatrix);
}

void RendererImpl::EndFrame() {
    if (!m_initialized) {
        return;
    }
    
    m_context->EndFrame();
}

void RendererImpl::SetWireframe(bool enable) {
    if (!m_initialized) {
        return;
    }
    
    m_wireframe = enable;
    m_terrain->SetWireframe(enable);
    bgfx::setDebug(m_wireframe ? BGFX_DEBUG_WIREFRAME : BGFX_DEBUG_NONE);
}

void RendererImpl::SetScale(float horizontalScale, float verticalScale) {
    m_scaleH = horizontalScale;
    m_scaleV = verticalScale;
    
    if (m_initialized && m_terrain) {
        // 考虑高度图的宽高比
        float aspectRatio = m_heightmap ? m_heightmap->GetAspectRatio() : 1.0f;
        m_terrain->SetTerrainScale(m_scaleH * aspectRatio, m_scaleV, m_scaleH);
    }
    
    printf("[RendererImpl] Scale set to: %.2f x %.2f\n", m_scaleH, m_scaleV);
}

const char* RendererImpl::GetLastError() const {
    return m_lastError.c_str();
}

} // namespace terrain