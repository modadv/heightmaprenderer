#pragma once

#include "uniforms.h"
#include "types.h"

#include <bgfx/bgfx.h>
#include <bimg/bimg.h>
#include <bx/file.h>
#include <entry/entry.h>

struct TextureOption {
    const char* name;
    const char* path;
};

struct LoadTimeRecord {
    float loadTimeMs;
    char heightmapName[64];
    char diffuseName[64];
    int64_t timestamp;
};

struct DMap {
    bx::FilePath pathToFile;
    float scale;
};

class HeightmapRenderer {
public:
    static constexpr int MAX_HEIGHTMAP_OPTIONS = 2;
    static constexpr int MAX_DIFFUSE_OPTIONS = 2;
    static constexpr int MAX_LOAD_HISTORY = 5;

    HeightmapRenderer();
    ~HeightmapRenderer();

    bool init(uint32_t width, uint32_t height);
    void shutdown();
    bool update(float deltaTime, const entry::MouseState& mouseState);

    // Configuration
    void setWireframe(bool enabled) { m_wireframe = enabled; }
    void setCulling(bool enabled) { m_cull = enabled; }
    void setFreeze(bool enabled) { m_freeze = enabled; }
    void setPrimitivePixelLength(float length) { m_primitivePixelLengthTarget = length; }
    void setShading(int shading) { m_shading = shading; }
    void setGpuSubdivision(int level);

    // Texture management
    bool loadHeightmap(int index);
    bool loadDiffuseTexture(int index);
    void reloadTextures();

    // Performance stats
    float getLoadTime() const { return m_loadTime; }
    float getCpuSmapTime() const { return m_cpuSmapGenTime; }
    float getGpuSmapTime() const { return m_gpuSmapGenTime; }

    // Initialization methods
    void loadPrograms();
    void loadTextures();
    void loadBuffers();
    void createAtomicCounters();
    void initTextureOptions();

    // Texture loading methods
    void loadDmapTexture();
    void loadSmapTexture();
    void loadSmapTextureGPU();
    void loadDiffuseTexture();

    // Buffer management
    void loadGeometryBuffers();
    void loadInstancedGeometryBuffers();
    void loadSubdivisionBuffers();

    // Rendering
    void configureUniforms();
    void updateTexturePaths();
    void renderTerrain(const float* viewMtx, const float* projMtx);

private:
    // Resources
    Uniforms m_uniforms;
    
    bgfx::ProgramHandle m_programsCompute[types::PROGRAM_COUNT];
    bgfx::ProgramHandle m_programsDraw[types::SHADING_COUNT];
    bgfx::TextureHandle m_textures[types::TEXTURE_COUNT];
    bgfx::UniformHandle m_samplers[types::SAMPLER_COUNT];
    bgfx::UniformHandle m_smapParamsHandle;

    // Buffers
    bgfx::DynamicIndexBufferHandle m_bufferSubd[2];
    bgfx::DynamicIndexBufferHandle m_bufferCulledSubd;
    bgfx::DynamicIndexBufferHandle m_bufferCounter;
    bgfx::IndexBufferHandle m_geometryIndices;
    bgfx::VertexBufferHandle m_geometryVertices;
    bgfx::VertexLayout m_geometryLayout;
    bgfx::IndexBufferHandle m_instancedGeometryIndices;
    bgfx::VertexBufferHandle m_instancedGeometryVertices;
    bgfx::VertexLayout m_instancedGeometryLayout;
    bgfx::IndirectBufferHandle m_dispatchIndirect;

    // Image data
    bimg::ImageContainer* m_dmap;

    // Configuration
    DMap m_dmapConfig;
    TextureOption m_heightmapOptions[MAX_HEIGHTMAP_OPTIONS];
    TextureOption m_diffuseOptions[MAX_DIFFUSE_OPTIONS];
    
    // State
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_instancedMeshVertexCount;
    uint32_t m_instancedMeshPrimitiveCount;
    
    int m_selectedHeightmap;
    int m_selectedDiffuse;
    int m_shading;
    int m_pingPong;
    
    float m_terrainAspectRatio;
    float m_primitivePixelLengthTarget;
    float m_fovy;
    
    bool m_restart;
    bool m_wireframe;
    bool m_cull;
    bool m_freeze;
    bool m_useGpuSmap;
    bool m_texturesNeedReload;

    // Performance tracking
    int64_t m_loadStartTime;
    bool m_firstFrameRendered;
    float m_loadTime;
    float m_cpuSmapGenTime;
    float m_gpuSmapGenTime;
    
    LoadTimeRecord m_loadHistory[MAX_LOAD_HISTORY];
    int m_loadHistoryCount;

    // Paths
    char m_heightmapPath[256];
    char m_diffuseTexturePath[256];
};
