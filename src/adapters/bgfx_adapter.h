// bgfx_adapter.h
#pragma once
#include <QObject>
#include <QTimer>
#include <QImage>
#include <QVariant>
#include "../renderer/bgfx_renderer.h"

class BgfxAdapter : public QObject
{
    Q_OBJECT
        Q_PROPERTY(QImage image READ image NOTIFY imageChanged)
public:
    explicit BgfxAdapter(void* nativeWindowHandle, const QVariant& glContext, QObject* parent = nullptr);
    ~BgfxAdapter() override;
    QImage image() const;

public slots:
    void resize(int width, int height);

signals:
    void imageChanged();

private slots:
    void render();

private:
    BgfxRenderer m_renderer;
    QTimer m_renderTimer;
    QImage m_image;

    bgfx::TextureHandle m_renderTargetTexture = BGFX_INVALID_HANDLE;
    bgfx::TextureHandle m_readbackTexture = BGFX_INVALID_HANDLE;
};