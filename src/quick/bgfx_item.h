// BgfxItem.h
#pragma once
#include <QQuickItem>
#include "../adapters/bgfx_adapter.h"

class BgfxItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(BgfxAdapter* adapter READ adapter WRITE setAdapter NOTIFY adapterChanged)
public:
    explicit BgfxItem(QQuickItem* parent = nullptr) : QQuickItem(parent) {
        setFlag(ItemHasContents, true);
    }
    BgfxAdapter* adapter() const { return m_adapter; }
    void setAdapter(BgfxAdapter* adapter);
signals:
    void adapterChanged();
protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;
private:
    BgfxAdapter* m_adapter = nullptr;
};