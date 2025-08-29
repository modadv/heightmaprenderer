// BgfxRenderer.h
#pragma once

#include <bgfx/bgfx.h>
#include <QVariant>

class BgfxRenderer
{
public:
    BgfxRenderer();
    ~BgfxRenderer();

    void init(void* nativeWindowHandle, const QVariant& glContext);
    void shutdown();
    bool isInitialized() const;

    // 修改: resize现在接收一个来自外部的、已包装的BGFX纹理句柄
    void resize(uint32_t width, uint32_t height, bgfx::TextureHandle colorTexture);
    bgfx::TextureHandle getOutputTexture() const;
    void render();

private:
    bool m_isInitialized = false;
    uint32_t m_width = 0;
    uint32_t m_height = 0;

    // --- 新增BGFX资源句柄 ---
    bgfx::FrameBufferHandle m_fbo;
    bgfx::TextureHandle m_fboColorTexture;
    bgfx::TextureHandle m_fboDepthTexture;

    // 用于渲染简单立方体的资源
    bgfx::ProgramHandle m_program;
    bgfx::VertexBufferHandle m_vbo;
    bgfx::IndexBufferHandle m_ibo;

    int64_t m_timeOffset;
};