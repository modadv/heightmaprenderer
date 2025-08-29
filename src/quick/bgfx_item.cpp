// BgfxItem.cpp
#include "bgfx_item.h"
#include "../adapters/bgfx_adapter.h"
#include <QQuickWindow>
#include <QSGImageNode> // 使用公有的 QSGImageNode
#include <QDebug>



void BgfxItem::setAdapter(BgfxAdapter* adapter)
{
    if (m_adapter == adapter) return;
    if (m_adapter) disconnect(m_adapter, &BgfxAdapter::imageChanged, this, &QQuickItem::update);

    m_adapter = adapter;
    
    if (m_adapter) {
        connect(m_adapter, &BgfxAdapter::imageChanged, this, &QQuickItem::update);
        connect(this, &QQuickItem::widthChanged, this, [=](){ if (m_adapter) m_adapter->resize(width(), height()); });
        connect(this, &QQuickItem::heightChanged, this, [=](){ if (m_adapter) m_adapter->resize(width(), height()); });
        if (width() > 0 && height() > 0)
            m_adapter->resize(width(), height());
    }
    emit adapterChanged();
    update();
}

QSGNode* BgfxItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
    QSGImageNode *node = static_cast<QSGImageNode *>(oldNode);
    if (!node) {
        // --- 核心修复：使用窗口的工厂方法创建节点 ---
        node = window()->createImageNode();
    }

    if (!m_adapter || m_adapter->image().isNull()) {
        if (node->texture()) {
            delete node->texture();
            node->setTexture(nullptr);
        }
        return node;
    }
    
    // createTextureFromImage返回的纹理会被node接管，无需手动删除
    QSGTexture *texture = window()->createTextureFromImage(m_adapter->image());
    if (texture) {
        node->setRect(0, 0, width(), height());
        node->setTexture(texture);
        node->setFiltering(QSGTexture::Linear);
        node->setOwnsTexture(true); // 明确告知Node拥有这个纹理的生命周期
    }
    return node;
}