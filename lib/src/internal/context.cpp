// lib/src/internal/context.cpp
#include "context.h"
#include <bgfx/platform.h>
#include <bx/bx.h>
#include <cstdio>

namespace terrain {
namespace internal {

RenderContext::RenderContext() {
}

RenderContext::~RenderContext() {
    if (m_initialized) {
        Shutdown();
    }
}

bool RenderContext::Initialize(uint32_t width, uint32_t height, void* nativeHandle) {
    if (m_initialized) {
        return false;
    }
    
    printf("[RenderContext] Initializing bgfx...\n");
    
    // 初始化bgfx
    bgfx::Init init;
    init.type = bgfx::RendererType::Count; // 自动选择
    init.vendorId = BGFX_PCI_ID_NONE;
    init.resolution.width = width;
    init.resolution.height = height;
    init.resolution.reset = BGFX_RESET_VSYNC;
    
    // 设置平台数据（如果提供了窗口句柄）
    if (nativeHandle) {
        init.platformData.nwh = nativeHandle;
        m_nativeHandle = nativeHandle;
    }
    
    if (!bgfx::init(init)) {
        printf("[RenderContext] Failed to initialize bgfx\n");
        return false;
    }
    
    m_width = width;
    m_height = height;
    
    // 设置视图清除状态
    bgfx::setViewClear(m_viewId
        , BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH
        , 0x303030ff
        , 1.0f
        , 0
    );
    
    // 设置视图矩形
    bgfx::setViewRect(m_viewId, 0, 0, uint16_t(width), uint16_t(height));
    
    printf("[RenderContext] Initialized successfully (%dx%d)\n", width, height);
    
    // 打印渲染器信息
    const bgfx::Caps* caps = bgfx::getCaps();
    printf("[RenderContext] Renderer: %s\n", bgfx::getRendererName(caps->rendererType));
    
    m_initialized = true;
    return true;
}

void RenderContext::Shutdown() {
    if (!m_initialized) {
        return;
    }
    
    printf("[RenderContext] Shutting down bgfx...\n");
    
    bgfx::shutdown();
    
    m_initialized = false;
    m_width = 0;
    m_height = 0;
    m_nativeHandle = nullptr;
}

void RenderContext::SetRenderTarget(const RenderTarget& target) {
    if (!m_initialized) {
        return;
    }
    
    // 如果窗口句柄改变，更新平台数据
    if (target.nativeHandle && target.nativeHandle != m_nativeHandle) {
        #ifdef _WIN32
        bgfx::PlatformData pd = {};
        pd.nwh = target.nativeHandle;
        bgfx::setPlatformData(pd);
        #endif
        
        m_nativeHandle = target.nativeHandle;
    }
    
    // 如果尺寸改变，重置
    if (target.width != m_width || target.height != m_height) {
        m_width = target.width;
        m_height = target.height;
        
        bgfx::reset(m_width, m_height, BGFX_RESET_VSYNC);
        bgfx::setViewRect(m_viewId, 0, 0, uint16_t(m_width), uint16_t(m_height));
        
        printf("[RenderContext] Resized to %dx%d\n", m_width, m_height);
    }
}

void RenderContext::BeginFrame() {
    // 清除
    bgfx::touch(m_viewId);
}

void RenderContext::EndFrame() {
    // 提交帧
    bgfx::frame();
}

} // namespace internal
} // namespace terrain