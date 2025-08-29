// BgfxAdapter.cpp (修正版)
#include "bgfx_adapter.h"
#include <QDebug>
#include <bgfx/bgfx.h> // 需要包含以使用bgfx::frame

BgfxAdapter::BgfxAdapter(void* nativeWindowHandle, const QVariant& glContext, QObject* parent) : QObject(parent)
{
    m_renderer.init(nativeWindowHandle, glContext);
    connect(&m_renderTimer, &QTimer::timeout, this, &BgfxAdapter::render);
    m_renderTimer.start(16);
}

BgfxAdapter::~BgfxAdapter() { m_renderer.shutdown(); }
QImage BgfxAdapter::image() const { return m_image; }

void BgfxAdapter::resize(int width, int height)
{
    if (width <= 0 || height <= 0) return;
    if (m_image.size() == QSize(width, height)) return;

    m_image = QImage(width, height, QImage::Format_ARGB32);
    if (bgfx::isValid(m_renderTargetTexture)) bgfx::destroy(m_renderTargetTexture);
    if (bgfx::isValid(m_readbackTexture)) bgfx::destroy(m_readbackTexture);

    // 1. 创建渲染目标纹理 (不变)
    m_renderTargetTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8, BGFX_TEXTURE_RT);

    // --- 核心最终修复：为回读纹理添加BGFX_TEXTURE_BLIT_DST标志 ---
    // 2. 创建用于CPU回读的中转纹理
    m_readbackTexture = bgfx::createTexture2D(width, height, false, 1, bgfx::TextureFormat::RGBA8,
        BGFX_TEXTURE_READ_BACK | BGFX_TEXTURE_BLIT_DST);
    // --- 结束修复 ---

    // 将渲染目标纹理传递给渲染器
    m_renderer.resize(width, height, m_renderTargetTexture);
}

void BgfxAdapter::render()
{
    if (!m_renderer.isInitialized() || !bgfx::isValid(m_renderTargetTexture)) {
        return;
    }

    // 1. BGFX在渲染目标纹理上完成绘制
    m_renderer.render();

    // 2. (Blit) 将渲染结果从渲染目标复制到回读纹理
    bgfx::blit(255, m_readbackTexture, 0, 0, m_renderTargetTexture);

    // 3. (Readback) 请求从回读纹理中读取数据到QImage的内存
    bgfx::readTexture(m_readbackTexture, m_image.bits());

    // 4. 提交所有指令
    bgfx::frame();

    // 5. 通知UI更新
    emit imageChanged();
}