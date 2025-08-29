#pragma once
#include "entry_p.h"

#include <bgfx/platform.h>

#include <bx/mutex.h>
#include <bx/handlealloc.h>
#include <bx/os.h>
#include <bx/thread.h>
#include <bx/timer.h>
#include <bx/uint32_t.h>

#include <tinystl/allocator.h>
#include <tinystl/string.h>
#include <tinystl/vector.h>
#include <QQuickWindow>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlContext>
#include <QOpenGLContext>

#include <memory>

namespace entry {

    // Qt QML specific configuration
    struct QtConfig {
        bool useOpenGL = true;
        bool vsync = true;
        int msaaLevel = 0;
        bgfx::RendererType::Enum renderer = bgfx::RendererType::Count; // auto-select
    };

    // 视口信息
    struct ViewportInfo
    {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        bool valid = false;
    };


    struct WindowParams
    {
        int32_t x = 100;
        int32_t y = 100;
        uint32_t width = 1280;
        uint32_t height = 720;
        const char* title = "BGFX";
        bool useQml = false;
    } ;


    Key::Enum translateKey(int qtKey);
    uint8_t translateModifiers(Qt::KeyboardModifiers modifiers);
    MouseButton::Enum translateMouseButton(Qt::MouseButton button);
    void ensureWindowCreated();
    QQuickWindow* createQmlRenderWindow(const QtConfig& config = {});
    bool setQmlWindowHandle(QQuickWindow* window);
    void* getQmlNativeWindowHandle(QQuickWindow* window);
    
    // BGFX Quick Item for QML - 作为占位符和事件处理
    class BgfxQuickItem : public QQuickItem
    {
        Q_OBJECT
        Q_PROPERTY(bool renderingEnabled READ renderingEnabled WRITE setRenderingEnabled NOTIFY renderingEnabledChanged)

    public:
        BgfxQuickItem(QQuickItem* parent = nullptr)
            : QQuickItem(parent)
            , m_renderingEnabled(true)
            , m_eventQueue(nullptr)
        {
            setAcceptedMouseButtons(Qt::AllButtons);
            setAcceptHoverEvents(true);
            setFlag(ItemAcceptsInputMethod, true);
            setFocus(true);

            // 监听位置和大小变化
            connect(this, &QQuickItem::xChanged, this, &BgfxQuickItem::updateViewport);
            connect(this, &QQuickItem::yChanged, this, &BgfxQuickItem::updateViewport);
            connect(this, &QQuickItem::widthChanged, this, &BgfxQuickItem::updateViewport);
            connect(this, &QQuickItem::heightChanged, this, &BgfxQuickItem::updateViewport);
        }

        bool renderingEnabled() const { return m_renderingEnabled; }
        void setRenderingEnabled(bool enabled)
        {
            if (m_renderingEnabled != enabled)
            {
                m_renderingEnabled = enabled;
                emit renderingEnabledChanged();
            }
        }

        void setEventQueue(EventQueue* queue)
        {
            m_eventQueue = queue;
        }

        // 获取在窗口中的实际位置和大小
        QRect getViewportRect() const
        {
            if (!window()) return QRect();

            QPointF globalPos = mapToScene(QPointF(0, 0));
            return QRect(globalPos.x(), globalPos.y(), width(), height());
        }

    signals:
        void renderingEnabledChanged();
        void viewportChanged(int x, int y, int width, int height);

    public slots:
        void updateViewport()
        {
            QRect rect = getViewportRect();
            if (rect.isValid())
            {
                emit viewportChanged(rect.x(), rect.y(), rect.width(), rect.height());

                // 通知 BGFX 视口变化
                if (m_eventQueue)
                {
                    // 发送自定义的视口变化事件
                    // 注意：这里我们发送的是控件的大小，而不是窗口的大小
                    m_eventQueue->postSizeEvent(kDefaultWindowHandle, rect.width(), rect.height());
                }
            }
        }

    protected:
        void keyPressEvent(QKeyEvent* event) override
        {
            if (m_eventQueue && !event->isAutoRepeat())
            {
                Key::Enum key = translateKey(event->key());
                if (key != Key::None)
                {
                    m_eventQueue->postKeyEvent(kDefaultWindowHandle, key,
                        translateModifiers(event->modifiers()), true);
                }
            }
            event->accept();
        }

        void keyReleaseEvent(QKeyEvent* event) override
        {
            if (m_eventQueue && !event->isAutoRepeat())
            {
                Key::Enum key = translateKey(event->key());
                if (key != Key::None)
                {
                    m_eventQueue->postKeyEvent(kDefaultWindowHandle, key,
                        translateModifiers(event->modifiers()), false);
                }
            }
            event->accept();
        }

        void mousePressEvent(QMouseEvent* event) override
        {
            if (m_eventQueue)
            {
                MouseButton::Enum button = translateMouseButton(event->button());
                if (button != MouseButton::None)
                {
                    // 发送相对于控件的坐标
                    m_eventQueue->postMouseEvent(kDefaultWindowHandle,
                        event->x(), event->y(), 0, button, true);
                }
            }
            forceActiveFocus();
            event->accept();
        }

        void mouseReleaseEvent(QMouseEvent* event) override
        {
            if (m_eventQueue)
            {
                MouseButton::Enum button = translateMouseButton(event->button());
                if (button != MouseButton::None)
                {
                    m_eventQueue->postMouseEvent(kDefaultWindowHandle,
                        event->x(), event->y(), 0, button, false);
                }
            }
            event->accept();
        }

        void mouseMoveEvent(QMouseEvent* event) override
        {
            if (m_eventQueue)
            {
                m_eventQueue->postMouseEvent(kDefaultWindowHandle,
                    event->x(), event->y(), 0);
            }
            event->accept();
        }

        void wheelEvent(QWheelEvent* event) override
        {
            if (m_eventQueue)
            {
                int delta = event->angleDelta().y() / 120;
                m_eventQueue->postMouseEvent(kDefaultWindowHandle,
                    event->x(), event->y(), delta);
            }
            event->accept();
        }

    private:
        bool m_renderingEnabled;
        EventQueue* m_eventQueue;
    };

    extern QGuiApplication* s_app;
    extern QQmlApplicationEngine* s_engine;
    extern BgfxQuickItem* s_bgfxItem;
    extern QWindow* s_window;
    extern QQuickWindow* s_quickWindow;
    extern EventQueue s_eventQueue;
    extern ViewportInfo s_viewport;
    extern WindowParams s_windowParams;
}
