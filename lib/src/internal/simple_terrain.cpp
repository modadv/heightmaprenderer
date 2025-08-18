// lib/src/internal/simple_terrain.cpp
#include "simple_terrain.h"
#include <bx/math.h>
#include <vector>
#include <cstdio>

// 嵌入简单的着色器代码
static const char* s_simpleVS = R"(
$input a_position, a_texcoord0
$output v_texcoord0, v_worldPos

#include <bgfx_shader.sh>

uniform vec4 u_terrainParams; // scaleX, scaleY, scaleZ, unused

void main()
{
    vec3 pos = a_position.xyz;
    
    // 从纹理坐标获取高度（简化版本）
    float height = 0.0;
    
    // 应用缩放
    pos.x *= u_terrainParams.x;
    pos.y = height * u_terrainParams.y;
    pos.z *= u_terrainParams.z;
    
    v_worldPos = pos;
    v_texcoord0 = a_texcoord0;
    
    gl_Position = mul(u_modelViewProj, vec4(pos, 1.0));
}
)";

static const char* s_simpleFS = R"(
$input v_texcoord0, v_worldPos

#include <bgfx_shader.sh>

SAMPLER2D(s_heightmap, 0);

void main()
{
    // 从高度图采样
    float height = texture2D(s_heightmap, v_texcoord0).r;
    
    // 简单的颜色映射
    vec3 color = vec3(height, height * 0.8, height * 0.6);
    
    gl_FragColor = vec4(color, 1.0);
}
)";

namespace terrain {
namespace internal {

struct PosTexVertex {
    float x, y, z;
    float u, v;
};

SimpleTerrain::SimpleTerrain() {
}

SimpleTerrain::~SimpleTerrain() {
    if (m_initialized) {
        Shutdown();
    }
}

bool SimpleTerrain::Initialize(int gridSizeX, int gridSizeZ) {
    if (m_initialized) {
        return false;
    }
    
    m_gridSizeX = gridSizeX;
    m_gridSizeZ = gridSizeZ;
    
    printf("[SimpleTerrain] Initializing with grid size %dx%d\n", m_gridSizeX, m_gridSizeZ);
    
    // 创建网格
    if (!CreateMesh()) {
        printf("[SimpleTerrain] Failed to create mesh\n");
        return false;
    }
    
    // 加载着色器
    if (!LoadShaders()) {
        printf("[SimpleTerrain] Failed to load shaders\n");
        return false;
    }
    
    m_initialized = true;
    return true;
}

void SimpleTerrain::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    printf("[SimpleTerrain] Shutting down\n");
    
    // 销毁缓冲区
    if (bgfx::isValid(m_vbh)) {
        bgfx::destroy(m_vbh);
        m_vbh = BGFX_INVALID_HANDLE;
    }
    
    if (bgfx::isValid(m_ibh)) {
        bgfx::destroy(m_ibh);
        m_ibh = BGFX_INVALID_HANDLE;
    }
    
    // 销毁着色器
    if (bgfx::isValid(m_program)) {
        bgfx::destroy(m_program);
        m_program = BGFX_INVALID_HANDLE;
    }
    
    // 销毁uniforms
    if (bgfx::isValid(m_terrainParams)) {
        bgfx::destroy(m_terrainParams);
        m_terrainParams = BGFX_INVALID_HANDLE;
    }
    
    if (bgfx::isValid(m_heightmapSampler)) {
        bgfx::destroy(m_heightmapSampler);
        m_heightmapSampler = BGFX_INVALID_HANDLE;
    }
    
    m_initialized = false;
}

bool SimpleTerrain::CreateMesh() {
    // 设置顶点布局
    m_layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();
    
    // 创建顶点数据
    std::vector<PosTexVertex> vertices;
    vertices.reserve((m_gridSizeX + 1) * (m_gridSizeZ + 1));
    
    for (int z = 0; z <= m_gridSizeZ; ++z) {
        for (int x = 0; x <= m_gridSizeX; ++x) {
            PosTexVertex vertex;
            
            // 位置（-0.5 到 0.5 范围）
            vertex.x = (float)x / m_gridSizeX - 0.5f;
            vertex.y = 0.0f;  // 高度将由着色器处理
            vertex.z = (float)z / m_gridSizeZ - 0.5f;
            
            // 纹理坐标（0 到 1）
            vertex.u = (float)x / m_gridSizeX;
            vertex.v = (float)z / m_gridSizeZ;
            
            vertices.push_back(vertex);
        }
    }
    
    // 创建索引数据
    std::vector<uint16_t> indices;
    indices.reserve(m_gridSizeX * m_gridSizeZ * 6);
    
    for (int z = 0; z < m_gridSizeZ; ++z) {
        for (int x = 0; x < m_gridSizeX; ++x) {
            uint16_t topLeft = z * (m_gridSizeX + 1) + x;
            uint16_t topRight = topLeft + 1;
            uint16_t bottomLeft = (z + 1) * (m_gridSizeX + 1) + x;
            uint16_t bottomRight = bottomLeft + 1;
            
            // 第一个三角形
            indices.push_back(topLeft);
            indices.push_back(bottomLeft);
            indices.push_back(topRight);
            
            // 第二个三角形
            indices.push_back(topRight);
            indices.push_back(bottomLeft);
            indices.push_back(bottomRight);
        }
    }
    
    // 创建GPU缓冲区
    const bgfx::Memory* vbMem = bgfx::copy(vertices.data(), vertices.size() * sizeof(PosTexVertex));
    m_vbh = bgfx::createVertexBuffer(vbMem, m_layout);
    
    const bgfx::Memory* ibMem = bgfx::copy(indices.data(), indices.size() * sizeof(uint16_t));
    m_ibh = bgfx::createIndexBuffer(ibMem);
    
    printf("[SimpleTerrain] Created mesh: %zu vertices, %zu indices\n", 
           vertices.size(), indices.size());
    
    return bgfx::isValid(m_vbh) && bgfx::isValid(m_ibh);
}

bool SimpleTerrain::LoadShaders() {
    // 使用原项目编译好的着色器（如果存在）
    // 这里暂时使用一个简单的fallback着色器
    
    // 创建uniforms
    m_terrainParams = bgfx::createUniform("u_terrainParams", bgfx::UniformType::Vec4);
    m_heightmapSampler = bgfx::createUniform("s_heightmap", bgfx::UniformType::Sampler);
    m_slopeMapSampler = bgfx::createUniform("s_slopemap", bgfx::UniformType::Sampler);
    m_diffuseSampler = bgfx::createUniform("s_diffuse", bgfx::UniformType::Sampler);
    
    // 暂时创建一个空的程序（后续会加载实际的着色器）
    // 这里我们需要使用bgfx::ProgramHandle的有效句柄
    // 实际项目中应该从文件加载编译好的着色器
    
    printf("[SimpleTerrain] Shaders initialized (placeholder)\n");
    
    return true;
}

void SimpleTerrain::SetTerrainScale(float scaleX, float scaleY, float scaleZ) {
    m_scaleX = scaleX;
    m_scaleY = scaleY;
    m_scaleZ = scaleZ;
}

void SimpleTerrain::Draw(const float* viewMatrix, const float* projMatrix) {
    if (!m_initialized) {
        return;
    }
    
    // 设置渲染状态
    uint64_t state = BGFX_STATE_WRITE_RGB 
                   | BGFX_STATE_WRITE_Z 
                   | BGFX_STATE_DEPTH_TEST_LESS
                   | BGFX_STATE_CULL_CCW;
    
    if (m_wireframe) {
        state |= BGFX_STATE_PT_TRISTRIP;  // 线框模式的近似
    }
    
    bgfx::setState(state);
    
    // 设置变换矩阵
    float mtx[16];
    bx::mtxIdentity(mtx);
    bgfx::setTransform(mtx);
    
    // 设置uniforms
    float terrainParams[4] = { m_scaleX, m_scaleY, m_scaleZ, 0.0f };
    bgfx::setUniform(m_terrainParams, terrainParams);
    
    // 绑定纹理
    if (bgfx::isValid(m_heightmapTexture)) {
        bgfx::setTexture(0, m_heightmapSampler, m_heightmapTexture);
    }
    
    if (bgfx::isValid(m_slopeMapTexture)) {
        bgfx::setTexture(1, m_slopeMapSampler, m_slopeMapTexture);
    }
    
    if (bgfx::isValid(m_diffuseTexture)) {
        bgfx::setTexture(2, m_diffuseSampler, m_diffuseTexture);
    }
    
    // 设置顶点和索引缓冲
    bgfx::setVertexBuffer(0, m_vbh);
    bgfx::setIndexBuffer(m_ibh);
    
    // 提交渲染（如果有有效的着色器程序）
    if (bgfx::isValid(m_program)) {
        bgfx::submit(0, m_program);
    } else {
        // 没有着色器时，仍然提交以避免错误
        // 这会显示为空白，但不会崩溃
        bgfx::discard();
    }
}

} // namespace internal
} // namespace terrain