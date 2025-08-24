#ifndef BGFXITEM_H
#define BGFXITEM_H

#include <QQuickItem>
#include <QTimer>

class BGFXItem : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
    
public:
    explicit BGFXItem(QQuickItem *parent = nullptr);
    ~BGFXItem();
    
    bool running() const { return m_running; }
    void setRunning(bool running);
    
signals:
    void runningChanged();
    void frameRendered();
    
protected:
    QSGNode *updatePaintNode(QSGNode *oldNode, UpdatePaintNodeData *) override;
    
private slots:
    void handleWindowChanged(QQuickWindow *window);
    void tick();
    
private:
    void sync();
    void cleanup();
    
    bool m_running;
    QTimer *m_timer;
    static bool s_bgfxInitialized;
};

#endif // BGFXITEM_H