// BgfxRenderer.cpp
#include "bgfx_renderer.h"
#include <common/bgfx_utils.h>
#include <bx/timer.h>
#include <bx/math.h>
#include <QtGlobal>
#include <QDebug>
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
#	include <bgfx/platform.h>
#elif BX_PLATFORM_OSX
#	include <bgfx/platform.h>
#elif BX_PLATFORM_WINDOWS
#	include <bgfx/platform.h>
#endif

static struct PosColorVertex
{
    float m_x;
    float m_y;
    float m_z;
    uint32_t m_abgr;
};

static PosColorVertex s_cubeVertices[] =
{
    {-1.0f,  1.0f,  1.0f, 0xff000000 }, { 1.0f,  1.0f,  1.0f, 0xff0000ff },
    {-1.0f, -1.0f,  1.0f, 0xff00ff00 }, { 1.0f, -1.0f,  1.0f, 0xff00ffff },
    {-1.0f,  1.0f, -1.0f, 0xffff0000 }, { 1.0f,  1.0f, -1.0f, 0xffff00ff },
    {-1.0f, -1.0f, -1.0f, 0xffffff00 }, { 1.0f, -1.0f, -1.0f, 0xffffffff },
};

static const uint16_t s_cubeTriList[] =
{
    0, 1, 2, 1, 3, 2, 4, 6, 5, 5, 6, 7,
    0, 2, 4, 4, 2, 6, 1, 5, 3, 5, 7, 3,
    0, 4, 1, 4, 5, 1, 2, 3, 6, 6, 3, 7,
};

BgfxRenderer::BgfxRenderer()
{
    m_fbo = BGFX_INVALID_HANDLE;
    m_fboColorTexture = BGFX_INVALID_HANDLE;
    m_fboDepthTexture = BGFX_INVALID_HANDLE;
    m_program = BGFX_INVALID_HANDLE;
    m_vbo = BGFX_INVALID_HANDLE;
    m_ibo = BGFX_INVALID_HANDLE;
}

BgfxRenderer::~BgfxRenderer() {}

void BgfxRenderer::init(void* nativeWindowHandle, const QVariant& glContext)
{
    if (m_isInitialized) return;

    qDebug() << "Initializing BGFX in single-threaded mode...";
    bgfx::Init init;
    init.type = bgfx::RendererType::OpenGL;
    init.resolution.width = 1;
    init.resolution.height = 1;
    init.resolution.reset = BGFX_RESET_VSYNC;

#if BX_PLATFORM_WINDOWS
    init.platformData.nwh = nativeWindowHandle;
    init.platformData.context = glContext.value<void*>();
#else
    // ... 其他平台实现 ...
#endif

    // --- 核心最终修复：在init之前调用，强制单线程模式 ---
    bgfx::renderFrame();
    // --- 结束修复 ---

    if (!bgfx::init(init)) {
        qCritical() << "BGFX initialization failed!";
        return;
    }
    // BGFX的日志现在应该会显示它运行在单线程模式
    qDebug() << "BGFX initialized successfully.";

    bgfx::VertexLayout layout;
    layout.begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
    m_vbo = bgfx::createVertexBuffer(bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices)), layout);
    m_ibo = bgfx::createIndexBuffer(bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList)));
    m_program = loadProgram("vs_cubes", "fs_cubes");
    m_timeOffset = bx::getHPCounter();
    m_isInitialized = true;
}

void BgfxRenderer::shutdown()
{
    if (m_isInitialized) {
        if (bgfx::isValid(m_program)) bgfx::destroy(m_program);
        if (bgfx::isValid(m_vbo)) bgfx::destroy(m_vbo);
        if (bgfx::isValid(m_ibo)) bgfx::destroy(m_ibo);

        bgfx::shutdown();
        m_isInitialized = false;
        qDebug() << "BGFX and resources shutdown.";
    }
}

bool BgfxRenderer::isInitialized() const
{
    return m_isInitialized;
}

void BgfxRenderer::resize(uint32_t width, uint32_t height, bgfx::TextureHandle colorTexture)
{
    if (!m_isInitialized) return;

    if (m_width == width && m_height == height && m_fboColorTexture.idx == colorTexture.idx) {
        return;
    }

    m_width = width;
    m_height = height;
    m_fboColorTexture = colorTexture;

    if (bgfx::isValid(m_fbo)) {
        bgfx::destroy(m_fbo);
        m_fbo = BGFX_INVALID_HANDLE;
    }

    if (m_width > 0 && m_height > 0 && bgfx::isValid(colorTexture)) {
        m_fboDepthTexture = bgfx::createTexture2D(m_width, m_height, false, 1, bgfx::TextureFormat::D24S8, BGFX_TEXTURE_RT_WRITE_ONLY);

        bgfx::TextureHandle textures[] = { colorTexture, m_fboDepthTexture };
        m_fbo = bgfx::createFrameBuffer(BX_COUNTOF(textures), textures, false);
    }
}

void BgfxRenderer::render()
{
    if (!m_isInitialized || !bgfx::isValid(m_fbo)) {
        return;
    }
    bgfx::setViewFrameBuffer(0, m_fbo);
    bgfx::setViewRect(0, 0, 0, uint16_t(m_width), uint16_t(m_height));
    bgfx::touch(0);
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x103010ff, 1.0f, 0);

    const bx::Vec3 at = { 0.0f, 0.0f,  0.0f };
    const bx::Vec3 eye = { 0.0f, 0.0f, -5.0f };
    float view[16];
    bx::mtxLookAt(view, eye, at);
    float proj[16];
    bx::mtxProj(proj, 60.0f, float(m_width) / float(m_height), 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
    bgfx::setViewTransform(0, view, proj);

    float mtx[16];
    float time = (float)((bx::getHPCounter() - m_timeOffset) / double(bx::getHPFrequency()));
    bx::mtxRotateXY(mtx, time, time * 0.37f);
    bgfx::setTransform(mtx);

    bgfx::setVertexBuffer(0, m_vbo);
    bgfx::setIndexBuffer(m_ibo);
    bgfx::setState(BGFX_STATE_DEFAULT);
    bgfx::submit(0, m_program);
}

// --- 添加这个缺失的函数实现 ---
bgfx::TextureHandle BgfxRenderer::getOutputTexture() const
{
    return m_fboColorTexture;
}