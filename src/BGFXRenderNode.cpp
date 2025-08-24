#include "BGFXRenderNode.h"
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <bx/math.h>
#include <bx/timer.h>

#ifdef _WIN32
#include <windows.h>
#include <GL/gl.h>
#endif

bgfx::VertexLayout BGFXRenderNode::PosColorVertex::ms_layout;

void BGFXRenderNode::PosColorVertex::init()
{
    ms_layout
        .begin()
        .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
        .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
        .end();
}

// 立方体数据
static BGFXRenderNode::PosColorVertex s_cubeVertices[] =
{
    {-1.0f,  1.0f,  1.0f, 0xff000000 },
    { 1.0f,  1.0f,  1.0f, 0xff0000ff },
    {-1.0f, -1.0f,  1.0f, 0xff00ff00 },
    { 1.0f, -1.0f,  1.0f, 0xff00ffff },
    {-1.0f,  1.0f, -1.0f, 0xffff0000 },
    { 1.0f,  1.0f, -1.0f, 0xffff00ff },
    {-1.0f, -1.0f, -1.0f, 0xffffff00 },
    { 1.0f, -1.0f, -1.0f, 0xffffffff },
};

static const uint16_t s_cubeTriList[] =
{
    0, 1, 2, 1, 3, 2,
    4, 6, 5, 5, 6, 7,
    0, 2, 4, 4, 2, 6,
    1, 5, 3, 5, 7, 3,
    0, 4, 1, 4, 5, 1,
    2, 3, 6, 6, 3, 7,
};

// 简单的顶点着色器 - GLSL
static const uint8_t vs_cubes_glsl[] = {
    0x56, 0x53, 0x48, 0x06, 0x00, 0x00, 0x00, 0x00, 0x01, 0x83, 0xf2, 0xe1, 0x01, 0x00, 0x0f, 0x75,
    0x5f, 0x6d, 0x6f, 0x64, 0x65, 0x6c, 0x56, 0x69, 0x65, 0x77, 0x50, 0x72, 0x6f, 0x6a, 0x04, 0x01,
    0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0xb9, 0x01, 0x00, 0x00, 0x61, 0x74, 0x74, 0x72,
    0x69, 0x62, 0x75, 0x74, 0x65, 0x20, 0x76, 0x65, 0x63, 0x33, 0x20, 0x61, 0x5f, 0x70, 0x6f, 0x73,
    0x69, 0x74, 0x69, 0x6f, 0x6e, 0x3b, 0x0a, 0x61, 0x74, 0x74, 0x72, 0x69, 0x62, 0x75, 0x74, 0x65,
    0x20, 0x76, 0x65, 0x63, 0x34, 0x20, 0x61, 0x5f, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x30, 0x3b, 0x0a,
    0x76, 0x61, 0x72, 0x79, 0x69, 0x6e, 0x67, 0x20, 0x76, 0x65, 0x63, 0x34, 0x20, 0x76, 0x5f, 0x63,
    0x6f, 0x6c, 0x6f, 0x72, 0x30, 0x3b, 0x0a, 0x75, 0x6e, 0x69, 0x66, 0x6f, 0x72, 0x6d, 0x20, 0x6d,
    0x61, 0x74, 0x34, 0x20, 0x75, 0x5f, 0x6d, 0x6f, 0x64, 0x65, 0x6c, 0x56, 0x69, 0x65, 0x77, 0x50,
    0x72, 0x6f, 0x6a, 0x3b, 0x0a, 0x76, 0x6f, 0x69, 0x64, 0x20, 0x6d, 0x61, 0x69, 0x6e, 0x20, 0x28,
    0x29, 0x0a, 0x7b, 0x0a, 0x20, 0x20, 0x76, 0x65, 0x63, 0x34, 0x20, 0x74, 0x6d, 0x70, 0x76, 0x61,
    0x72, 0x5f, 0x31, 0x3b, 0x0a, 0x20, 0x20, 0x74, 0x6d, 0x70, 0x76, 0x61, 0x72, 0x5f, 0x31, 0x2e,
    0x77, 0x20, 0x3d, 0x20, 0x31, 0x2e, 0x30, 0x3b, 0x0a, 0x20, 0x20, 0x74, 0x6d, 0x70, 0x76, 0x61,
    0x72, 0x5f, 0x31, 0x2e, 0x78, 0x79, 0x7a, 0x20, 0x3d, 0x20, 0x61, 0x5f, 0x70, 0x6f, 0x73, 0x69,
    0x74, 0x69, 0x6f, 0x6e, 0x3b, 0x0a, 0x20, 0x20, 0x67, 0x6c, 0x5f, 0x50, 0x6f, 0x73, 0x69, 0x74,
    0x69, 0x6f, 0x6e, 0x20, 0x3d, 0x20, 0x28, 0x75, 0x5f, 0x6d, 0x6f, 0x64, 0x65, 0x6c, 0x56, 0x69,
    0x65, 0x77, 0x50, 0x72, 0x6f, 0x6a, 0x20, 0x2a, 0x20, 0x74, 0x6d, 0x70, 0x76, 0x61, 0x72, 0x5f,
    0x31, 0x29, 0x3b, 0x0a, 0x20, 0x20, 0x76, 0x5f, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x30, 0x20, 0x3d,
    0x20, 0x61, 0x5f, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x30, 0x3b, 0x0a, 0x7d, 0x0a, 0x0a, 0x00
};

// 简单的片段着色器 - GLSL
static const uint8_t fs_cubes_glsl[] = {
    0x46, 0x53, 0x48, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7f, 0x00, 0x00, 0x00, 0x76, 0x61, 0x72, 0x79, 0x69, 0x6e,
    0x67, 0x20, 0x76, 0x65, 0x63, 0x34, 0x20, 0x76, 0x5f, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x30, 0x3b,
    0x0a, 0x76, 0x6f, 0x69, 0x64, 0x20, 0x6d, 0x61, 0x69, 0x6e, 0x20, 0x28, 0x29, 0x0a, 0x7b, 0x0a,
    0x20, 0x20, 0x67, 0x6c, 0x5f, 0x46, 0x72, 0x61, 0x67, 0x43, 0x6f, 0x6c, 0x6f, 0x72, 0x20, 0x3d,
    0x20, 0x76, 0x5f, 0x63, 0x6f, 0x6c, 0x6f, 0x72, 0x30, 0x3b, 0x0a, 0x7d, 0x0a, 0x0a, 0x00
};

BGFXRenderNode::BGFXRenderNode(QQuickWindow *window)
    : m_window(window)
    , m_vbh(BGFX_INVALID_HANDLE)
    , m_ibh(BGFX_INVALID_HANDLE)
    , m_program(BGFX_INVALID_HANDLE)
    , m_time(0.0f)
    , m_initialized(false)
{
}

BGFXRenderNode::~BGFXRenderNode()
{
    releaseResources();
}

bool BGFXRenderNode::initialize()
{
    if (m_initialized) {
        return true;
    }
    
    // 确保在正确的OpenGL上下文中
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        ctx = m_window->openglContext();
        if (!ctx) {
            return false;
        }
        ctx->makeCurrent(m_window);
    }
    
    // 获取窗口句柄
    WId winId = m_window->winId();
    void* nativeHandle = reinterpret_cast<void*>(winId);
    
    // 设置平台数据
    bgfx::PlatformData pd;
    memset(&pd, 0, sizeof(pd));
    
#ifdef _WIN32
    // Windows平台
    HWND hwnd = (HWND)nativeHandle;
    pd.ndt = NULL;
    pd.nwh = hwnd;
    
    // 获取OpenGL上下文句柄
    QVariant nativeContext = ctx->nativeHandle();
    if (nativeContext.isValid() && nativeContext.canConvert<void*>()) {
        pd.context = nativeContext.value<void*>();
    } else {
        // 如果无法获取，尝试使用当前的GL上下文
        pd.context = wglGetCurrentContext();
    }
    
    pd.backBuffer = NULL;
    pd.backBufferDS = NULL;
#elif __APPLE__
    pd.nwh = nativeHandle;
    QVariant nativeContext = ctx->nativeHandle();
    if (nativeContext.isValid() && nativeContext.canConvert<void*>()) {
        pd.context = nativeContext.value<void*>();
    }
#else // Linux
    pd.ndt = nullptr;
    pd.nwh = nativeHandle;
    QVariant nativeContext = ctx->nativeHandle();
    if (nativeContext.isValid() && nativeContext.canConvert<void*>()) {
        pd.context = nativeContext.value<void*>();
    }
#endif
    
    bgfx::setPlatformData(pd);
    
    // 调用renderFrame让bgfx使用单线程模式
    bgfx::renderFrame();
    
    // 初始化bgfx
    bgfx::Init init;
    init.type = bgfx::RendererType::OpenGL;
    init.vendorId = BGFX_PCI_ID_NONE;
    init.resolution.width = uint32_t(m_window->width());
    init.resolution.height = uint32_t(m_window->height());
    init.resolution.reset = BGFX_RESET_VSYNC;
    init.callback = nullptr;
    init.allocator = nullptr;
    
    if (!bgfx::init(init)) {
        return false;
    }
    
    // 启用调试文本
    bgfx::setDebug(BGFX_DEBUG_TEXT);
    
    // 创建资源
    createResources();
    
    m_initialized = true;
    return true;
}

void BGFXRenderNode::createResources()
{
    PosColorVertex::init();
    
    // 创建顶点缓冲
    m_vbh = bgfx::createVertexBuffer(
        bgfx::makeRef(s_cubeVertices, sizeof(s_cubeVertices)),
        PosColorVertex::ms_layout
    );
    
    // 创建索引缓冲
    m_ibh = bgfx::createIndexBuffer(
        bgfx::makeRef(s_cubeTriList, sizeof(s_cubeTriList))
    );
    
    // 创建着色器
    bgfx::ShaderHandle vsh = BGFX_INVALID_HANDLE;
    bgfx::ShaderHandle fsh = BGFX_INVALID_HANDLE;
    
    // 根据渲染器类型选择着色器
    bgfx::RendererType::Enum type = bgfx::getCaps()->rendererType;
    
    if (type == bgfx::RendererType::OpenGL || 
        type == bgfx::RendererType::OpenGLES) {
        // 使用预编译的GLSL着色器
        vsh = bgfx::createShader(bgfx::makeRef(vs_cubes_glsl, sizeof(vs_cubes_glsl)));
        fsh = bgfx::createShader(bgfx::makeRef(fs_cubes_glsl, sizeof(fs_cubes_glsl)));
    }
    
    // 如果着色器创建失败，尝试创建默认程序
    if (!bgfx::isValid(vsh) || !bgfx::isValid(fsh)) {
        // 创建一个简单的默认程序
        m_program = BGFX_INVALID_HANDLE;
    } else {
        m_program = bgfx::createProgram(vsh, fsh, true);
    }
}

void BGFXRenderNode::destroyResources()
{
    if (bgfx::isValid(m_vbh)) {
        bgfx::destroy(m_vbh);
        m_vbh = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_ibh)) {
        bgfx::destroy(m_ibh);
        m_ibh = BGFX_INVALID_HANDLE;
    }
    if (bgfx::isValid(m_program)) {
        bgfx::destroy(m_program);
        m_program = BGFX_INVALID_HANDLE;
    }
}

void BGFXRenderNode::render(const RenderState *state)
{
    if (!m_initialized) {
        if (!initialize()) {
            return;
        }
    }
    
    // 确保OpenGL上下文是当前的
    QOpenGLContext *ctx = QOpenGLContext::currentContext();
    if (!ctx) {
        return;
    }
    
    // 保存OpenGL状态
    QOpenGLFunctions *f = ctx->functions();
    
    GLint oldProgram;
    f->glGetIntegerv(GL_CURRENT_PROGRAM, &oldProgram);
    GLint oldViewport[4];
    f->glGetIntegerv(GL_VIEWPORT, oldViewport);
    GLboolean oldDepthTest = f->glIsEnabled(GL_DEPTH_TEST);
    GLboolean oldCullFace = f->glIsEnabled(GL_CULL_FACE);
    GLboolean oldBlend = f->glIsEnabled(GL_BLEND);
    
    // 设置bgfx视口
    const QRect viewport = m_rect.toRect();
    bgfx::setViewRect(0, 0, 0, 
                     uint16_t(viewport.width()), uint16_t(viewport.height()));
    
    // 清除背景
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    
    // 调试文本
    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(0, 1, 0x0f, "Qt/BGFX Integration");
    bgfx::dbgTextPrintf(0, 2, 0x0f, "Resolution: %dx%d", viewport.width(), viewport.height());
    
    // 设置视图和投影矩阵
    float view[16];
    float proj[16];
    
    const bx::Vec3 at = { 0.0f, 0.0f,  0.0f };
    const bx::Vec3 eye = { 0.0f, 0.0f, -5.0f };
    const bx::Vec3 up = { 0.0f, 1.0f,  0.0f };
    
    bx::mtxLookAt(view, eye, at, up);
    
    const float aspect = float(viewport.width()) / float(viewport.height());
    bx::mtxProj(proj, 60.0f, aspect, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
    
    bgfx::setViewTransform(0, view, proj);
    
    // 如果有有效的程序，渲染立方体
    if (bgfx::isValid(m_program)) {
        float mtx[16];
        bx::mtxRotateXY(mtx, m_time, m_time * 0.37f);
        
        bgfx::setTransform(mtx);
        bgfx::setVertexBuffer(0, m_vbh);
        bgfx::setIndexBuffer(m_ibh);
        
        uint64_t state = 0
            | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_WRITE_Z
            | BGFX_STATE_DEPTH_TEST_LESS
            | BGFX_STATE_CULL_CW
            | BGFX_STATE_MSAA;
        
        bgfx::setState(state);
        bgfx::submit(0, m_program);
    } else {
        // 如果没有着色器程序，至少清除视图
        bgfx::touch(0);
    }
    
    // 提交帧
    bgfx::frame();
    
    m_time += 0.016f;
    
    // 恢复OpenGL状态
    f->glUseProgram(oldProgram);
    f->glViewport(oldViewport[0], oldViewport[1], oldViewport[2], oldViewport[3]);
    
    if (oldDepthTest) f->glEnable(GL_DEPTH_TEST);
    else f->glDisable(GL_DEPTH_TEST);
    
    if (oldCullFace) f->glEnable(GL_CULL_FACE);
    else f->glDisable(GL_CULL_FACE);
    
    if (oldBlend) f->glEnable(GL_BLEND);
    else f->glDisable(GL_BLEND);
}

void BGFXRenderNode::releaseResources()
{
    if (m_initialized) {
        destroyResources();
        m_initialized = false;
    }
}

QSGRenderNode::StateFlags BGFXRenderNode::changedStates() const
{
    return BlendState | ScissorState | StencilState | DepthState | 
           ColorState | CullState | ViewportState;
}

QSGRenderNode::RenderingFlags BGFXRenderNode::flags() const
{
    return BoundedRectRendering | DepthAwareRendering;
}

QRectF BGFXRenderNode::rect() const
{
    return m_rect;
}

void BGFXRenderNode::setRect(const QRectF &rect)
{
    m_rect = rect;
}

void BGFXRenderNode::sync()
{
    // 同步操作
}