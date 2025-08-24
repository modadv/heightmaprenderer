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

// 立方体顶点数据
static BGFXRenderNode::PosColorVertex s_cubeVertices[] =
{
    {-1.0f,  1.0f,  1.0f, 0xff0000ff },  // 蓝色
    { 1.0f,  1.0f,  1.0f, 0xff00ff00 },  // 绿色
    {-1.0f, -1.0f,  1.0f, 0xff00ffff },  // 青色
    { 1.0f, -1.0f,  1.0f, 0xffff0000 },  // 红色
    {-1.0f,  1.0f, -1.0f, 0xffff00ff },  // 品红
    { 1.0f,  1.0f, -1.0f, 0xffffff00 },  // 黄色
    {-1.0f, -1.0f, -1.0f, 0xffffffff },  // 白色
    { 1.0f, -1.0f, -1.0f, 0xff808080 },  // 灰色
};

static const uint16_t s_cubeTriList[] =
{
    // 前面
    0, 1, 2,
    1, 3, 2,
    // 后面
    4, 6, 5,
    5, 6, 7,
    // 左面
    0, 2, 4,
    4, 2, 6,
    // 右面
    1, 5, 3,
    5, 7, 3,
    // 上面
    0, 4, 1,
    4, 5, 1,
    // 下面
    2, 3, 6,
    6, 3, 7,
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
    HWND hwnd = (HWND)nativeHandle;
    pd.ndt = NULL;
    pd.nwh = hwnd;
    pd.context = wglGetCurrentContext();
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
    
    // 使用单线程模式
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
    
    // 获取渲染器信息
    const bgfx::Caps* caps = bgfx::getCaps();
    
    // 创建资源
    createResources();
    
    m_initialized = true;
    return true;
}

void BGFXRenderNode::createResources()
{
    // 初始化顶点布局
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
    
    // 不创建着色器程序，使用内置调试渲染
    m_program = BGFX_INVALID_HANDLE;
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
    
    // 获取渲染区域
    const QRect viewport = m_rect.toRect();
    const uint16_t viewId = 0;
    
    // 设置视口和清除
    bgfx::setViewRect(viewId, viewport.x(), viewport.y(), 
                     uint16_t(viewport.width()), uint16_t(viewport.height()));
    
    // 使用不同的颜色作为背景，以便看到渲染效果
    uint32_t clearColor = 0x443355ff;  // 深紫色背景
    bgfx::setViewClear(viewId, 
                      BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 
                      clearColor,
                      1.0f, 
                      0);
    
    // 调试文本 - 显示更多信息
    bgfx::dbgTextClear();
    bgfx::dbgTextPrintf(0, 1, 0x4f, "Qt/BGFX Integration");
    bgfx::dbgTextPrintf(0, 2, 0x6f, "Resolution: %dx%d", viewport.width(), viewport.height());
    bgfx::dbgTextPrintf(0, 3, 0x5f, "Time: %.2f", m_time);
    bgfx::dbgTextPrintf(0, 4, 0x3f, "Renderer: %s", bgfx::getRendererName(bgfx::getCaps()->rendererType));
    
    // 显示简单的ASCII艺术立方体
    bgfx::dbgTextPrintf(0, 6, 0x1f, "     +-------+");
    bgfx::dbgTextPrintf(0, 7, 0x2f, "    /|      /|");
    bgfx::dbgTextPrintf(0, 8, 0x3f, "   + +-----+ |");
    bgfx::dbgTextPrintf(0, 9, 0x4f, "   |/     /| +");
    bgfx::dbgTextPrintf(0,10, 0x5f, "   +-----+ |/");
    bgfx::dbgTextPrintf(0,11, 0x6f, "   |     | +");
    bgfx::dbgTextPrintf(0,12, 0x7f, "   |     |/");
    bgfx::dbgTextPrintf(0,13, 0x8f, "   +-----+");
    
    // 设置视图和投影矩阵
    float view[16];
    float proj[16];
    
    // 动态相机
    float eyeX = sinf(m_time * 0.5f) * 5.0f;
    float eyeY = 3.0f;
    float eyeZ = cosf(m_time * 0.5f) * 5.0f;
    
    const bx::Vec3 at = { 0.0f, 0.0f,  0.0f };
    const bx::Vec3 eye = { eyeX, eyeY, eyeZ };
    const bx::Vec3 up = { 0.0f, 1.0f,  0.0f };
    
    bx::mtxLookAt(view, eye, at, up);
    
    const float aspect = float(viewport.width()) / float(viewport.height());
    bx::mtxProj(proj, 60.0f, aspect, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
    
    bgfx::setViewTransform(viewId, view, proj);
    
    // 渲染多个立方体
    if (bgfx::isValid(m_vbh) && bgfx::isValid(m_ibh)) {
        // 渲染中心立方体
        float mtx[16];
        bx::mtxRotateXY(mtx, m_time * 0.5f, m_time * 0.37f);
        bgfx::setTransform(mtx);
        
        // 设置顶点和索引缓冲
        bgfx::setVertexBuffer(0, m_vbh);
        bgfx::setIndexBuffer(m_ibh);
        
        // 设置渲染状态
        uint64_t state = 0
            | BGFX_STATE_WRITE_RGB
            | BGFX_STATE_WRITE_A
            | BGFX_STATE_WRITE_Z
            | BGFX_STATE_DEPTH_TEST_LESS
            | BGFX_STATE_CULL_CW
            | BGFX_STATE_MSAA;
        
        bgfx::setState(state);
        
        // 提交渲染 - 使用INVALID_HANDLE让bgfx使用内置程序
        bgfx::submit(viewId, BGFX_INVALID_HANDLE);
        
        // 渲染第二个立方体
        float mtx2[16];
        bx::mtxRotateXY(mtx2, -m_time * 0.3f, m_time * 0.5f);
        mtx2[12] = 3.0f;  // X偏移
        bgfx::setTransform(mtx2);
        bgfx::setVertexBuffer(0, m_vbh);
        bgfx::setIndexBuffer(m_ibh);
        bgfx::setState(state);
        bgfx::submit(viewId, BGFX_INVALID_HANDLE);
        
        // 渲染第三个立方体
        float mtx3[16];
        bx::mtxRotateXY(mtx3, m_time * 0.7f, -m_time * 0.3f);
        mtx3[12] = -3.0f;  // X偏移
        bgfx::setTransform(mtx3);
        bgfx::setVertexBuffer(0, m_vbh);
        bgfx::setIndexBuffer(m_ibh);
        bgfx::setState(state);
        bgfx::submit(viewId, BGFX_INVALID_HANDLE);
    }
    
    // 确保至少触摸视图
    bgfx::touch(viewId);
    
    // 提交帧
    uint32_t frameNumber = bgfx::frame();
    
    // 更新时间
    m_time += 0.016f;
    if (m_time > 2.0f * 3.14159f) {
        m_time -= 2.0f * 3.14159f;
    }
    
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