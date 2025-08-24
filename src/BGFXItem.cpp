#include "BGFXItem.h"
#include "BGFXRenderNode.h"
#include <QQuickWindow>
#include <bgfx/bgfx.h>

bool BGFXItem::s_bgfxInitialized = false;

BGFXItem::BGFXItem(QQuickItem *parent)
    : QQuickItem(parent)
    , m_running(true)
    , m_timer(new QTimer(this))
{
    setFlag(ItemHasContents, true);
    
    connect(this, &QQuickItem::windowChanged, this, &BGFXItem::handleWindowChanged);
    connect(m_timer, &QTimer::timeout, this, &BGFXItem::tick);
    
    m_timer->setInterval(16); // ~60 FPS
    if (m_running) {
        m_timer->start();
    }
}

BGFXItem::~BGFXItem()
{
    cleanup();
    
    // 关闭bgfx（只在最后一个实例时）
    if (s_bgfxInitialized) {
        bgfx::shutdown();
        s_bgfxInitialized = false;
    }
}

void BGFXItem::setRunning(bool running)
{
    if (m_running != running) {
        m_running = running;
        
        if (m_running) {
            m_timer->start();
        } else {
            m_timer->stop();
        }
        
        emit runningChanged();
    }
}

QSGNode *BGFXItem::updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *)
{
    BGFXRenderNode *node = static_cast<BGFXRenderNode *>(oldNode);
    
    if (!node) {
        node = new BGFXRenderNode(window());
    }
    
    // 更新节点的渲染区域
    node->setRect(boundingRect());
    node->sync();
    
    emit frameRendered();
    
    return node;
}

void BGFXItem::handleWindowChanged(QQuickWindow *window)
{
    if (window) {
        connect(window, &QQuickWindow::beforeSynchronizing, 
                this, &BGFXItem::sync, Qt::DirectConnection);
        connect(window, &QQuickWindow::sceneGraphInvalidated, 
                this, &BGFXItem::cleanup, Qt::DirectConnection);
        
        // 设置为不清除背景
        window->setClearBeforeRendering(false);
    }
}

void BGFXItem::tick()
{
    if (window() && m_running) {
        update();
    }
}

void BGFXItem::sync()
{
    // 同步操作
}

void BGFXItem::cleanup()
{
    // 清理操作
}