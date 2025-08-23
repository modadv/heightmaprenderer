#include "heightmap_renderer.h"
#include "patch_tables.h"
#include "types.h"
#include "../common/bgfx_utils.h"
#include "../common/camera.h"
#include "../common/imgui/imgui.h"

#include <bx/math.h>
#include <bx/timer.h>
#include <cstdio>

HeightmapRenderer::HeightmapRenderer()
    : m_dmap(nullptr)
    , m_width(0)
    , m_height(0)
    , m_instancedMeshVertexCount(0)
    , m_instancedMeshPrimitiveCount(0)
    , m_selectedHeightmap(0)
    , m_selectedDiffuse(0)
    , m_shading(types::PROGRAM_TERRAIN)
    , m_pingPong(0)
    , m_terrainAspectRatio(1.0f)
    , m_primitivePixelLengthTarget(1.0f)
    , m_fovy(60.0f)
    , m_restart(true)
    , m_wireframe(false)
    , m_cull(true)
    , m_freeze(false)
    , m_useGpuSmap(true)
    , m_texturesNeedReload(false)
    , m_loadStartTime(0)
    , m_firstFrameRendered(false)
    , m_loadTime(0.0f)
    , m_cpuSmapGenTime(0.0f)
    , m_gpuSmapGenTime(0.0f)
    , m_loadHistoryCount(0)
{
    // Initialize invalid handles
    for (uint32_t i = 0; i < types::PROGRAM_COUNT; ++i) {
        m_programsCompute[i] = BGFX_INVALID_HANDLE;
    }
    for (uint32_t i = 0; i < types::SHADING_COUNT; ++i) {
        m_programsDraw[i] = BGFX_INVALID_HANDLE;
    }
    for (uint32_t i = 0; i < types::TEXTURE_COUNT; ++i) {
        m_textures[i] = BGFX_INVALID_HANDLE;
    }
    for (uint32_t i = 0; i < types::SAMPLER_COUNT; ++i) {
        m_samplers[i] = BGFX_INVALID_HANDLE;
    }

    m_bufferSubd[0] = BGFX_INVALID_HANDLE;
    m_bufferSubd[1] = BGFX_INVALID_HANDLE;
    m_bufferCulledSubd = BGFX_INVALID_HANDLE;
    m_bufferCounter = BGFX_INVALID_HANDLE;
    m_geometryIndices = BGFX_INVALID_HANDLE;
    m_geometryVertices = BGFX_INVALID_HANDLE;
    m_instancedGeometryIndices = BGFX_INVALID_HANDLE;
    m_instancedGeometryVertices = BGFX_INVALID_HANDLE;
    m_dispatchIndirect = BGFX_INVALID_HANDLE;
    m_smapParamsHandle = BGFX_INVALID_HANDLE;

    // Initialize paths
    m_heightmapPath[0] = '\0';
    m_diffuseTexturePath[0] = '\0';
}

HeightmapRenderer::~HeightmapRenderer() {
    shutdown();
}

bool HeightmapRenderer::init(uint32_t width, uint32_t height) {
    m_width = width;
    m_height = height;
    m_loadStartTime = bx::getHPCounter();
    m_firstFrameRendered = false;

    // Initialize texture options
    initTextureOptions();

    // Set default selections
    m_selectedHeightmap = 0;
    m_selectedDiffuse = 0;

    // Update texture paths
    updateTexturePaths();

    // Set default DMap configuration
    m_dmapConfig = { bx::FilePath(m_heightmapPath), 0.80f };

    try {
        loadPrograms();
        loadTextures();
        loadBuffers();
        createAtomicCounters();

        m_dispatchIndirect = bgfx::createIndirectBuffer(2);

        return true;
    }
    catch (...) {
        shutdown();
        return false;
    }
}

void HeightmapRenderer::shutdown() {
    m_uniforms.destroy();

    if (bgfx::isValid(m_bufferCounter)) {
        bgfx::destroy(m_bufferCounter);
        m_bufferCounter = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_bufferCulledSubd)) {
        bgfx::destroy(m_bufferCulledSubd);
        m_bufferCulledSubd = BGFX_INVALID_HANDLE;
    }

    for (int i = 0; i < 2; ++i) {
        if (bgfx::isValid(m_bufferSubd[i])) {
            bgfx::destroy(m_bufferSubd[i]);
            m_bufferSubd[i] = BGFX_INVALID_HANDLE;
        }
    }

    if (bgfx::isValid(m_dispatchIndirect)) {
        bgfx::destroy(m_dispatchIndirect);
        m_dispatchIndirect = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_geometryIndices)) {
        bgfx::destroy(m_geometryIndices);
        m_geometryIndices = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_geometryVertices)) {
        bgfx::destroy(m_geometryVertices);
        m_geometryVertices = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_instancedGeometryIndices)) {
        bgfx::destroy(m_instancedGeometryIndices);
        m_instancedGeometryIndices = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_instancedGeometryVertices)) {
        bgfx::destroy(m_instancedGeometryVertices);
        m_instancedGeometryVertices = BGFX_INVALID_HANDLE;
    }

    if (bgfx::isValid(m_smapParamsHandle)) {
        bgfx::destroy(m_smapParamsHandle);
        m_smapParamsHandle = BGFX_INVALID_HANDLE;
    }

    for (uint32_t i = 0; i < types::PROGRAM_COUNT; ++i) {
        if (bgfx::isValid(m_programsCompute[i])) {
            bgfx::destroy(m_programsCompute[i]);
            m_programsCompute[i] = BGFX_INVALID_HANDLE;
        }
    }

    for (uint32_t i = 0; i < types::SHADING_COUNT; ++i) {
        if (bgfx::isValid(m_programsDraw[i])) {
            bgfx::destroy(m_programsDraw[i]);
            m_programsDraw[i] = BGFX_INVALID_HANDLE;
        }
    }

    for (uint32_t i = 0; i < types::SAMPLER_COUNT; ++i) {
        if (bgfx::isValid(m_samplers[i])) {
            bgfx::destroy(m_samplers[i]);
            m_samplers[i] = BGFX_INVALID_HANDLE;
        }
    }

    for (uint32_t i = 0; i < types::TEXTURE_COUNT; ++i) {
        if (bgfx::isValid(m_textures[i])) {
            bgfx::destroy(m_textures[i]);
            m_textures[i] = BGFX_INVALID_HANDLE;
        }
    }

    if (m_dmap) {
        bimg::imageFree(m_dmap);
        m_dmap = nullptr;
    }
}

bool HeightmapRenderer::update(float deltaTime, const entry::MouseState& mouseState) {
    // Calculate first frame loading time
    if (!m_firstFrameRendered) {
        int64_t now = bx::getHPCounter();
        m_loadTime = float((now - m_loadStartTime) / double(bx::getHPFrequency()) * 1000.0);
        m_firstFrameRendered = true;
        printf("Loading time: %.2f ms\n", m_loadTime);
        
        // Add to load history
        if (m_loadHistoryCount < MAX_LOAD_HISTORY) {
            m_loadHistory[m_loadHistoryCount].loadTimeMs = m_loadTime;
            m_loadHistory[m_loadHistoryCount].timestamp = now;
            bx::strCopy(m_loadHistory[m_loadHistoryCount].heightmapName,
                sizeof(m_loadHistory[m_loadHistoryCount].heightmapName),
                m_heightmapOptions[m_selectedHeightmap].name);
            bx::strCopy(m_loadHistory[m_loadHistoryCount].diffuseName,
                sizeof(m_loadHistory[m_loadHistoryCount].diffuseName),
                m_diffuseOptions[m_selectedDiffuse].name);
            m_loadHistoryCount++;
        } else {
            // Shift array and add new record
            for (int i = 0; i < MAX_LOAD_HISTORY - 1; i++) {
                m_loadHistory[i] = m_loadHistory[i + 1];
            }
            m_loadHistory[MAX_LOAD_HISTORY - 1].loadTimeMs = m_loadTime;
            m_loadHistory[MAX_LOAD_HISTORY - 1].timestamp = now;
            bx::strCopy(m_loadHistory[MAX_LOAD_HISTORY - 1].heightmapName,
                sizeof(m_loadHistory[MAX_LOAD_HISTORY - 1].heightmapName),
                m_heightmapOptions[m_selectedHeightmap].name);
            bx::strCopy(m_loadHistory[MAX_LOAD_HISTORY - 1].diffuseName,
                sizeof(m_loadHistory[MAX_LOAD_HISTORY - 1].diffuseName),
                m_diffuseOptions[m_selectedDiffuse].name);
        }
    }

    // Handle texture reload
    if (m_texturesNeedReload) {
        m_texturesNeedReload = false;
        m_loadStartTime = bx::getHPCounter();
        m_firstFrameRendered = false;

        // Update DMap path
        m_dmapConfig.pathToFile = bx::FilePath(m_heightmapPath);

        // Clean up old textures
        for (uint32_t i = 0; i < types::TEXTURE_COUNT; ++i) {
            if (bgfx::isValid(m_textures[i])) {
                bgfx::destroy(m_textures[i]);
                m_textures[i] = BGFX_INVALID_HANDLE;
            }
        }

        // Reload textures
        loadDmapTexture();
        if (m_useGpuSmap) {
            loadSmapTextureGPU();
        } else {
            loadSmapTexture();
        }
        loadDiffuseTexture();

        // Update geometry
        if (bgfx::isValid(m_geometryVertices)) {
            bgfx::destroy(m_geometryVertices);
        }
        if (bgfx::isValid(m_geometryIndices)) {
            bgfx::destroy(m_geometryIndices);
        }
        loadGeometryBuffers();

        m_restart = true;
    }

    // Configure uniforms
    configureUniforms();

    // Get camera matrices
    float viewMtx[16];
    float projMtx[16];
    cameraGetViewMtx(viewMtx);
    bx::mtxProj(projMtx, m_fovy, float(m_width) / float(m_height), 0.0001f, 2000.0f, bgfx::getCaps()->homogeneousDepth);

    // Set view transforms
    bgfx::setViewTransform(0, viewMtx, projMtx);
    bgfx::setViewRect(1, 0, 0, uint16_t(m_width), uint16_t(m_height));
    bgfx::setViewTransform(1, viewMtx, projMtx);

    // Render terrain
    renderTerrain(viewMtx, projMtx);

    return true;
}

void HeightmapRenderer::setGpuSubdivision(int level) {
    if (level != int(m_uniforms.gpuSubd)) {
        m_restart = true;
        m_uniforms.gpuSubd = float(level);
    }
}

bool HeightmapRenderer::loadHeightmap(int index) {
    if (index >= 0 && index < MAX_HEIGHTMAP_OPTIONS) {
        m_selectedHeightmap = index;
        m_selectedDiffuse = index; // Keep them in sync
        updateTexturePaths();
        m_texturesNeedReload = true;
        return true;
    }
    return false;
}

bool HeightmapRenderer::loadDiffuseTexture(int index) {
    if (index >= 0 && index < MAX_DIFFUSE_OPTIONS) {
        m_selectedDiffuse = index;
        updateTexturePaths();
        m_texturesNeedReload = true;
        return true;
    }
    return false;
}

void HeightmapRenderer::reloadTextures() {
    m_texturesNeedReload = true;
}

void HeightmapRenderer::loadPrograms() {
    m_samplers[types::TERRAIN_DMAP_SAMPLER] = bgfx::createUniform("u_DmapSampler", bgfx::UniformType::Sampler);
    m_samplers[types::TERRAIN_SMAP_SAMPLER] = bgfx::createUniform("u_SmapSampler", bgfx::UniformType::Sampler);
    m_samplers[types::TERRAIN_DIFFUSE_SAMPLER] = bgfx::createUniform("u_DiffuseSampler", bgfx::UniformType::Sampler);

    m_uniforms.init();

    m_programsDraw[types::PROGRAM_TERRAIN] = loadProgram("vs_terrain_render", "fs_terrain_render");
    m_programsDraw[types::PROGRAM_TERRAIN_NORMAL] = loadProgram("vs_terrain_render", "fs_terrain_render_normal");

    m_programsCompute[types::PROGRAM_SUBD_CS_LOD] = bgfx::createProgram(loadShader("cs_terrain_lod"), true);
    m_programsCompute[types::PROGRAM_UPDATE_INDIRECT] = bgfx::createProgram(loadShader("cs_terrain_update_indirect"), true);
    m_programsCompute[types::PROGRAM_UPDATE_DRAW] = bgfx::createProgram(loadShader("cs_terrain_update_draw"), true);
    m_programsCompute[types::PROGRAM_INIT_INDIRECT] = bgfx::createProgram(loadShader("cs_terrain_init"), true);
    m_programsCompute[types::PROGRAM_GENERATE_SMAP] = bgfx::createProgram(loadShader("cs_generate_smap"), true);
    
    m_smapParamsHandle = bgfx::createUniform("u_smapParams", bgfx::UniformType::Vec4);
}

void HeightmapRenderer::loadTextures() {
    loadDmapTexture();
    if (m_useGpuSmap) {
        loadSmapTextureGPU();
    } else {
        loadSmapTexture();
    }
    loadDiffuseTexture();
}

void HeightmapRenderer::loadBuffers() {
    loadSubdivisionBuffers();
    loadGeometryBuffers();
    loadInstancedGeometryBuffers();
}

void HeightmapRenderer::createAtomicCounters() {
    m_bufferCounter = bgfx::createDynamicIndexBuffer(3, BGFX_BUFFER_INDEX32 | BGFX_BUFFER_COMPUTE_READ_WRITE);
}

void HeightmapRenderer::initTextureOptions() {
    m_heightmapOptions[0] = { "0049", "textures/0049_16bit.png" };
    m_heightmapOptions[1] = { "1972", "textures/1972_16bit.png" };

    m_diffuseOptions[0] = { "0049", "textures/0049.jpg" };
    m_diffuseOptions[1] = { "1972", "textures/1972.png" };
}

void HeightmapRenderer::loadDmapTexture() {
    m_dmap = imageLoad(m_dmapConfig.pathToFile.getCPtr(), bgfx::TextureFormat::R16);

    if (!m_dmap) {
        printf("Failed to load heightmap: %s\n", m_dmapConfig.pathToFile.getCPtr());

        const bgfx::Memory* mem = bgfx::alloc(sizeof(uint16_t));
        uint16_t* defaultHeightData = (uint16_t*)mem->data;
        *defaultHeightData = 0;

        m_textures[types::TEXTURE_DMAP] = bgfx::createTexture2D(
            1, 1, false, 1, bgfx::TextureFormat::R16,
            BGFX_TEXTURE_NONE, mem
        );

        m_terrainAspectRatio = 1.0f;
        return;
    }

    if (m_dmap->m_height > 0) {
        m_terrainAspectRatio = (float)m_dmap->m_width / (float)m_dmap->m_height;
    } else {
        m_terrainAspectRatio = 1.0f;
    }

    m_textures[types::TEXTURE_DMAP] = bgfx::createTexture2D(
        (uint16_t)m_dmap->m_width,
        (uint16_t)m_dmap->m_height,
        false,
        1,
        bgfx::TextureFormat::R16,
        BGFX_TEXTURE_NONE,
        bgfx::makeRef(m_dmap->m_data, m_dmap->m_size)
    );
}

void HeightmapRenderer::loadSmapTexture() {
    int64_t startTime = bx::getHPCounter();
    
    if (!m_dmap || m_dmap->m_width == 0 || m_dmap->m_height == 0) {
        const bgfx::Memory* mem = bgfx::alloc(2 * sizeof(float));
        float* defaultSlopeData = (float*)mem->data;
        defaultSlopeData[0] = 0.0f;
        defaultSlopeData[1] = 0.0f;

        m_textures[types::TEXTURE_SMAP] = bgfx::createTexture2D(
            1, 1, false, 1, bgfx::TextureFormat::RG32F,
            BGFX_TEXTURE_NONE, mem
        );
        m_cpuSmapGenTime = 0.0f;
        return;
    }

    int w = m_dmap->m_width;
    int h = m_dmap->m_height;
    const uint16_t* texels = (const uint16_t*)m_dmap->m_data;
    int mipcnt = m_dmap->m_numMips;

    const bgfx::Memory* mem = bgfx::alloc(w * h * 2 * sizeof(float));
    float* smap = (float*)mem->data;

    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            int i1 = bx::max(0, i - 1);
            int i2 = bx::min(w - 1, i + 1);
            int j1 = bx::max(0, j - 1);
            int j2 = bx::min(h - 1, j + 1);
            uint16_t px_l = texels[i1 + w * j];
            uint16_t px_r = texels[i2 + w * j];
            uint16_t px_b = texels[i + w * j1];
            uint16_t px_t = texels[i + w * j2];
            float z_l = (float)px_l / 65535.0f;
            float z_r = (float)px_r / 65535.0f;
            float z_b = (float)px_b / 65535.0f;
            float z_t = (float)px_t / 65535.0f;
            float slope_x = (float)w * 0.5f * (z_r - z_l);
            float slope_y = (float)h * 0.5f * (z_t - z_b);

            smap[2 * (i + w * j)] = slope_x;
            smap[1 + 2 * (i + w * j)] = slope_y;
        }
    }

    m_textures[types::TEXTURE_SMAP] = bgfx::createTexture2D(
        (uint16_t)w, (uint16_t)h, mipcnt > 1, 1, bgfx::TextureFormat::RG32F,
        BGFX_TEXTURE_NONE, mem
    );

    int64_t endTime = bx::getHPCounter();
    m_cpuSmapGenTime = float((endTime - startTime) / double(bx::getHPFrequency()) * 1000.0);
    printf("CPU SMap generation time: %.2f ms\n", m_cpuSmapGenTime);
}

void HeightmapRenderer::loadSmapTextureGPU() {
    int64_t startTime = bx::getHPCounter();
    
    if (!m_dmap || m_dmap->m_width == 0 || m_dmap->m_height == 0) {
        const bgfx::Memory* mem = bgfx::alloc(2 * sizeof(float));
        float* defaultSlopeData = (float*)mem->data;
        defaultSlopeData[0] = 0.0f;
        defaultSlopeData[1] = 0.0f;

        m_textures[types::TEXTURE_SMAP] = bgfx::createTexture2D(
            1, 1, false, 1, bgfx::TextureFormat::RG32F,
            BGFX_TEXTURE_NONE, mem
        );
        m_gpuSmapGenTime = 0.0f;
        return;
    }

    uint16_t w = static_cast<uint16_t>(m_dmap->m_width);
    uint16_t h = static_cast<uint16_t>(m_dmap->m_height);
    int mipcnt = m_dmap->m_numMips;

    m_textures[types::TEXTURE_SMAP] = bgfx::createTexture2D(
        w, h, mipcnt > 1, 1, bgfx::TextureFormat::RG32F,
        BGFX_TEXTURE_COMPUTE_WRITE
    );

    float smapParams[4] = { (float)w, (float)h, m_terrainAspectRatio, 1.0f };
    bgfx::setUniform(m_smapParamsHandle, smapParams);

    bgfx::setTexture(0, m_samplers[types::TERRAIN_DMAP_SAMPLER], m_textures[types::TEXTURE_DMAP], 
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

    bgfx::setImage(1, m_textures[types::TEXTURE_SMAP], 0, bgfx::Access::Write, bgfx::TextureFormat::RGBA32F);

    uint16_t groupsX = (w + 15) / 16;
    uint16_t groupsY = (h + 15) / 16;
    bgfx::dispatch(0, m_programsCompute[types::PROGRAM_GENERATE_SMAP], groupsX, groupsY, 1);
    bgfx::frame();

    int64_t endTime = bx::getHPCounter();
    m_gpuSmapGenTime = float((endTime - startTime) / double(bx::getHPFrequency()) * 1000.0);
    printf("GPU SMap generation time: %.2f ms (for %dx%d heightmap)\n", m_gpuSmapGenTime, w, h);

    if (m_cpuSmapGenTime > 0.0f) {
        float speedup = m_cpuSmapGenTime / m_gpuSmapGenTime;
        printf("GPU vs CPU speedup: %.1fx\n", speedup);
    }
}

void HeightmapRenderer::loadDiffuseTexture() {
    const char* filePath = m_diffuseTexturePath;
    uint64_t textureFlags = BGFX_TEXTURE_NONE | BGFX_SAMPLER_UVW_BORDER
        | BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC | BGFX_SAMPLER_MIP_SHIFT;

    m_textures[types::TEXTURE_DIFFUSE] = loadTexture(filePath, textureFlags);

    if (!bgfx::isValid(m_textures[types::TEXTURE_DIFFUSE])) {
        BX_TRACE("Failed to load diffuse texture: %s, using default texture", filePath);

        const bgfx::Memory* mem = bgfx::alloc(4);
        uint8_t* data = mem->data;
        data[0] = data[1] = data[2] = 128;
        data[3] = 255;

        m_textures[types::TEXTURE_DIFFUSE] = bgfx::createTexture2D(
            1, 1, false, 1, bgfx::TextureFormat::RGBA8,
            BGFX_TEXTURE_NONE, mem
        );
    } else {
        BX_TRACE("Loaded diffuse texture: %s", filePath);
    }
}

void HeightmapRenderer::loadGeometryBuffers() {
    const float halfWidth = m_terrainAspectRatio;
    const float halfHeight = 1.0f;

    const float vertices[] = {
        -halfWidth, -halfHeight, 0.0f, 1.0f,
        +halfWidth, -halfHeight, 0.0f, 1.0f,
        +halfWidth, +halfHeight, 0.0f, 1.0f,
        -halfWidth, +halfHeight, 0.0f, 1.0f,
    };

    const uint32_t indices[] = { 0, 1, 3, 2, 3, 1 };

    m_geometryLayout.begin().add(bgfx::Attrib::Position, 4, bgfx::AttribType::Float).end();

    m_geometryVertices = bgfx::createVertexBuffer(
        bgfx::copy(vertices, sizeof(vertices)),
        m_geometryLayout,
        BGFX_BUFFER_COMPUTE_READ
    );
    
    m_geometryIndices = bgfx::createIndexBuffer(
        bgfx::copy(indices, sizeof(indices)),
        BGFX_BUFFER_COMPUTE_READ | BGFX_BUFFER_INDEX32
    );
}

void HeightmapRenderer::loadInstancedGeometryBuffers() {
    const float* vertices;
    const uint32_t* indexes;

    switch (int32_t(m_uniforms.gpuSubd)) {
    case 0:
        m_instancedMeshVertexCount = 3;
        m_instancedMeshPrimitiveCount = 1;
        vertices = tables::s_verticesL0;
        indexes = tables::s_indexesL0;
        break;
    case 1:
        m_instancedMeshVertexCount = 6;
        m_instancedMeshPrimitiveCount = 4;
        vertices = tables::s_verticesL1;
        indexes = tables::s_indexesL1;
        break;
    case 2:
        m_instancedMeshVertexCount = 15;
        m_instancedMeshPrimitiveCount = 16;
        vertices = tables::s_verticesL2;
        indexes = tables::s_indexesL2;
        break;
    default:
        m_instancedMeshVertexCount = 45;
        m_instancedMeshPrimitiveCount = 64;
        vertices = tables::s_verticesL3;
        indexes = tables::s_indexesL3;
        break;
    }

    m_instancedGeometryLayout
        .begin()
        .add(bgfx::Attrib::TexCoord0, 2, bgfx::AttribType::Float)
        .end();

    m_instancedGeometryVertices = bgfx::createVertexBuffer(
        bgfx::makeRef(vertices, sizeof(float) * 2 * m_instancedMeshVertexCount),
        m_instancedGeometryLayout
    );

    m_instancedGeometryIndices = bgfx::createIndexBuffer(
        bgfx::makeRef(indexes, sizeof(uint32_t) * m_instancedMeshPrimitiveCount * 3),
        BGFX_BUFFER_INDEX32
    );
}

void HeightmapRenderer::loadSubdivisionBuffers() {
    const uint32_t bufferCapacity = 1 << 27;

    m_bufferSubd[types::BUFFER_SUBD] = bgfx::createDynamicIndexBuffer(
        bufferCapacity,
        BGFX_BUFFER_COMPUTE_READ_WRITE | BGFX_BUFFER_INDEX32
    );

    m_bufferSubd[types::BUFFER_SUBD + 1] = bgfx::createDynamicIndexBuffer(
        bufferCapacity,
        BGFX_BUFFER_COMPUTE_READ_WRITE | BGFX_BUFFER_INDEX32
    );

    m_bufferCulledSubd = bgfx::createDynamicIndexBuffer(
        bufferCapacity,
        BGFX_BUFFER_COMPUTE_READ_WRITE | BGFX_BUFFER_INDEX32
    );
}

void HeightmapRenderer::configureUniforms() {
    float lodFactor = 2.0f * bx::tan(bx::toRad(m_fovy) / 2.0f)
        / m_width * (1 << int(m_uniforms.gpuSubd))
        * m_primitivePixelLengthTarget;

    m_uniforms.lodFactor = lodFactor;
    m_uniforms.dmapFactor = m_dmapConfig.scale;
    m_uniforms.cull = m_cull ? 1.0f : 0.0f;
    m_uniforms.freeze = m_freeze ? 1.0f : 0.0f;
    m_uniforms.terrainHalfWidth = m_terrainAspectRatio;
    m_uniforms.terrainHalfHeight = 1.0f;
}

void HeightmapRenderer::updateTexturePaths() {
    bx::strCopy(m_heightmapPath, sizeof(m_heightmapPath),
        m_heightmapOptions[m_selectedHeightmap].path);
    bx::strCopy(m_diffuseTexturePath, sizeof(m_diffuseTexturePath),
        m_diffuseOptions[m_selectedDiffuse].path);
}

void HeightmapRenderer::renderTerrain(const float* viewMtx, const float* projMtx) {
    bgfx::touch(0);
    bgfx::touch(1);

    float model[16];
    bx::mtxRotateX(model, bx::toRad(90));

    m_uniforms.submit();

    // Update subdivision buffers
    if (m_restart) {
        m_pingPong = 1;

        if (bgfx::isValid(m_instancedGeometryVertices)) {
            bgfx::destroy(m_instancedGeometryVertices);
        }
        if (bgfx::isValid(m_instancedGeometryIndices)) {
            bgfx::destroy(m_instancedGeometryIndices);
        }
        if (bgfx::isValid(m_bufferSubd[types::BUFFER_SUBD])) {
            bgfx::destroy(m_bufferSubd[types::BUFFER_SUBD]);
        }
        if (bgfx::isValid(m_bufferSubd[types::BUFFER_SUBD + 1])) {
            bgfx::destroy(m_bufferSubd[types::BUFFER_SUBD + 1]);
        }
        if (bgfx::isValid(m_bufferCulledSubd)) {
            bgfx::destroy(m_bufferCulledSubd);
        }

        loadInstancedGeometryBuffers();
        loadSubdivisionBuffers();

        // Initialize indirect
        bgfx::setBuffer(1, m_bufferSubd[m_pingPong], bgfx::Access::ReadWrite);
        bgfx::setBuffer(2, m_bufferCulledSubd, bgfx::Access::ReadWrite);
        bgfx::setBuffer(3, m_dispatchIndirect, bgfx::Access::ReadWrite);
        bgfx::setBuffer(4, m_bufferCounter, bgfx::Access::ReadWrite);
        bgfx::setBuffer(8, m_bufferSubd[1 - m_pingPong], bgfx::Access::ReadWrite);
        bgfx::dispatch(0, m_programsCompute[types::PROGRAM_INIT_INDIRECT], 1, 1, 1);

        m_restart = false;
    } else {
        // Update batch
        bgfx::setBuffer(3, m_dispatchIndirect, bgfx::Access::ReadWrite);
        bgfx::setBuffer(4, m_bufferCounter, bgfx::Access::ReadWrite);
        bgfx::dispatch(0, m_programsCompute[types::PROGRAM_UPDATE_INDIRECT], 1, 1, 1);
    }

    // Subdivision LOD computation
    bgfx::setBuffer(1, m_bufferSubd[m_pingPong], bgfx::Access::ReadWrite);
    bgfx::setBuffer(2, m_bufferCulledSubd, bgfx::Access::ReadWrite);
    bgfx::setBuffer(4, m_bufferCounter, bgfx::Access::ReadWrite);
    bgfx::setBuffer(6, m_geometryVertices, bgfx::Access::Read);
    bgfx::setBuffer(7, m_geometryIndices, bgfx::Access::Read);
    bgfx::setBuffer(8, m_bufferSubd[1 - m_pingPong], bgfx::Access::Read);
    bgfx::setTransform(model);

    bgfx::setTexture(0, m_samplers[types::TERRAIN_DMAP_SAMPLER], m_textures[types::TEXTURE_DMAP], 
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);

    m_uniforms.submit();
    bgfx::dispatch(0, m_programsCompute[types::PROGRAM_SUBD_CS_LOD], m_dispatchIndirect, 1);

    // Update draw
    bgfx::setBuffer(3, m_dispatchIndirect, bgfx::Access::ReadWrite);
    bgfx::setBuffer(4, m_bufferCounter, bgfx::Access::ReadWrite);
    m_uniforms.submit();
    bgfx::dispatch(1, m_programsCompute[types::PROGRAM_UPDATE_DRAW], 1, 1, 1);

    // Render terrain
    bgfx::setTexture(0, m_samplers[types::TERRAIN_DMAP_SAMPLER], m_textures[types::TEXTURE_DMAP], 
        BGFX_SAMPLER_U_CLAMP | BGFX_SAMPLER_V_CLAMP);
    bgfx::setTexture(1, m_samplers[types::TERRAIN_SMAP_SAMPLER], m_textures[types::TEXTURE_SMAP], 
        BGFX_SAMPLER_MIN_ANISOTROPIC | BGFX_SAMPLER_MAG_ANISOTROPIC);

    if (bgfx::isValid(m_textures[types::TEXTURE_DIFFUSE])) {
        uint32_t diffuseSamplerFlags = BGFX_SAMPLER_UVW_MIRROR
            | BGFX_SAMPLER_MIN_ANISOTROPIC
            | BGFX_SAMPLER_MAG_ANISOTROPIC
            | BGFX_SAMPLER_MIP_POINT;

        bgfx::setTexture(5, m_samplers[types::TERRAIN_DIFFUSE_SAMPLER], m_textures[types::TEXTURE_DIFFUSE], 
            diffuseSamplerFlags);
    }

    bgfx::setTransform(model);
    bgfx::setVertexBuffer(0, m_instancedGeometryVertices);
    bgfx::setIndexBuffer(m_instancedGeometryIndices);
    bgfx::setBuffer(2, m_bufferCulledSubd, bgfx::Access::Read);
    bgfx::setBuffer(3, m_geometryVertices, bgfx::Access::Read);
    bgfx::setBuffer(4, m_geometryIndices, bgfx::Access::Read);
    bgfx::setState(BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS);

    m_uniforms.submit();
    bgfx::submit(1, m_programsDraw[m_shading], m_dispatchIndirect);

    m_pingPong = 1 - m_pingPong;
}
