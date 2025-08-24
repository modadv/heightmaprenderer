#ifndef BGFXRENDERNODE_H
#define BGFXRENDERNODE_H

#include <QSGRenderNode>
#include <QQuickWindow>
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>

class BGFXRenderNode : public QSGRenderNode
{
public:
    // 将PosColorVertex移到public部分
    struct PosColorVertex
    {
        float x, y, z;
        uint32_t abgr;
        
        static void init();
        static bgfx::VertexLayout ms_layout;
    };
    
    BGFXRenderNode(QQuickWindow *window);
    ~BGFXRenderNode();
    
    void render(const RenderState *state) override;
    void releaseResources() override;
    StateFlags changedStates() const override;
    RenderingFlags flags() const override;
    QRectF rect() const override;
    
    void setRect(const QRectF &rect);
    bool initialize();
    void sync();
    
private:
    void createResources();
    void destroyResources();
    
    QQuickWindow *m_window;
    QRectF m_rect;
    
    bgfx::VertexBufferHandle m_vbh;
    bgfx::IndexBufferHandle m_ibh;
    bgfx::ProgramHandle m_program;
    
    float m_time;
    bool m_initialized;
};

#endif // BGFXRENDERNODE_H