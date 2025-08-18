// lib/src/internal/simple_terrain.h
#pragma once

#include <bgfx/bgfx.h>
#include <memory>
#include <cstdint>

namespace terrain {
namespace internal {

// 简单的地形渲染器
// 使用固定网格，不包含LOD等高级功能
class SimpleTerrain {
public:
    SimpleTerrain();
    ~SimpleTerrain();
    
    // 初始化
    bool Initialize(int gridSizeX = 64, int gridSizeZ = 64);
    void Shutdown();
    
    // 设置纹理
    void SetHeightmapTexture(bgfx::TextureHandle texture) { m_heightmapTexture = texture; }
    void SetSlopeMapTexture(bgfx::TextureHandle texture) { m_slopeMapTexture = texture; }
    void SetDiffuseTexture(bgfx::TextureHandle texture) { m_diffuseTexture = texture; }
    
    // 设置参数
    void SetTerrainScale(float scaleX, float scaleY, float scaleZ);
    void SetWireframe(bool enable) { m_wireframe = enable; }
    
    // 渲染
    void Draw(const float* viewMatrix, const float* projMatrix);
    
    // 检查是否初始化
    bool IsInitialized() const { return m_initialized; }
    
private:
    bool CreateMesh();
    bool LoadShaders();
    
    bool m_initialized = false;
    
    // 网格参数
    int m_gridSizeX = 64;
    int m_gridSizeZ = 64;
    
    // 缩放
    float m_scaleX = 100.0f;
    float m_scaleY = 30.0f;
    float m_scaleZ = 100.0f;
    
    // 渲染状态
    bool m_wireframe = false;
    
    // 顶点和索引缓冲
    bgfx::VertexBufferHandle m_vbh = BGFX_INVALID_HANDLE;
    bgfx::IndexBufferHandle m_ibh = BGFX_INVALID_HANDLE;
    bgfx::VertexLayout m_layout;
    
    // 着色器程序
    bgfx::ProgramHandle m_program = BGFX_INVALID_HANDLE;
    
    // Uniforms
    bgfx::UniformHandle m_terrainParams = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_heightmapSampler = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_slopeMapSampler = BGFX_INVALID_HANDLE;
    bgfx::UniformHandle m_diffuseSampler = BGFX_INVALID_HANDLE;
    
    // 纹理
    bgfx::TextureHandle m_heightmapTexture = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_slopeMapTexture = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_diffuseTexture = BGFX_INVALID_HANDLE;
};

} // namespace internal
} // namespace terrain