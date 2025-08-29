#include "common/entry/entry_p.h"

#include <bgfx/platform.h>
#include <bgfx/bgfx.h>
#include <bx/thread.h>
#include <bx/os.h>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QQuickItem>
#include <QQmlContext>
#include <QOpenGLContext>

#if BX_PLATFORM_WINDOWS
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#elif BX_PLATFORM_LINUX
#include <X11/Xlib.h>
#include <qpa/qplatformnativeinterface.h>
#endif

namespace entry
{
    void getViewport(int& x, int& y, int& width, int& height);
    // 键盘和鼠标映射函数（保持不变）
    static Key::Enum translateKey(int qtKey)
    {
        if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z)
            return Key::Enum(Key::KeyA + (qtKey - Qt::Key_A));
        if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9)
            return Key::Enum(Key::Key0 + (qtKey - Qt::Key_0));
        if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F12)
            return Key::Enum(Key::F1 + (qtKey - Qt::Key_F1));

        switch (qtKey)
        {
        case Qt::Key_Escape:     return Key::Esc;
        case Qt::Key_Return:     return Key::Return;
        case Qt::Key_Tab:        return Key::Tab;
        case Qt::Key_Backspace:  return Key::Backspace;
        case Qt::Key_Space:      return Key::Space;
        case Qt::Key_Up:         return Key::Up;
        case Qt::Key_Down:       return Key::Down;
        case Qt::Key_Left:       return Key::Left;
        case Qt::Key_Right:      return Key::Right;
        case Qt::Key_Insert:     return Key::Insert;
        case Qt::Key_Delete:     return Key::Delete;
        case Qt::Key_Home:       return Key::Home;
        case Qt::Key_End:        return Key::End;
        case Qt::Key_PageUp:     return Key::PageUp;
        case Qt::Key_PageDown:   return Key::PageDown;
        case Qt::Key_Print:      return Key::Print;
        case Qt::Key_Plus:       return Key::Plus;
        case Qt::Key_Minus:      return Key::Minus;
        default:                 return Key::None;
        }
    }

    static uint8_t translateModifiers(Qt::KeyboardModifiers modifiers)
    {
        uint8_t result = 0;
        if (modifiers & Qt::ControlModifier) result |= Modifier::LeftCtrl;
        if (modifiers & Qt::ShiftModifier)   result |= Modifier::LeftShift;
        if (modifiers & Qt::AltModifier)     result |= Modifier::LeftAlt;
        if (modifiers & Qt::MetaModifier)    result |= Modifier::LeftMeta;
        return result;
    }

    static MouseButton::Enum translateMouseButton(Qt::MouseButton button)
    {
        switch (button)
        {
        case Qt::LeftButton:   return MouseButton::Left;
        case Qt::RightButton:  return MouseButton::Right;
        case Qt::MiddleButton: return MouseButton::Middle;
        default:               return MouseButton::None;
        }
    }
    // BGFX Quick Item for QML - 作为占位符和事件处理
    class BgfxItem : public QQuickItem
    {
        Q_OBJECT
            Q_PROPERTY(bool renderingEnabled READ renderingEnabled WRITE setRenderingEnabled NOTIFY renderingEnabledChanged)

    public:
        BgfxItem(QQuickItem* parent = nullptr)
            : QQuickItem(parent)
            , m_renderingEnabled(true)
            , m_eventQueue(nullptr)
        {
            setAcceptedMouseButtons(Qt::AllButtons);
            setAcceptHoverEvents(true);
            setFlag(ItemAcceptsInputMethod, true);
            setFocus(true);

            // 监听位置和大小变化
            connect(this, &QQuickItem::xChanged, this, &BgfxItem::updateViewport);
            connect(this, &QQuickItem::yChanged, this, &BgfxItem::updateViewport);
            connect(this, &QQuickItem::widthChanged, this, &BgfxItem::updateViewport);
            connect(this, &QQuickItem::heightChanged, this, &BgfxItem::updateViewport);
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
                printf("BgfxItem::updateViewport() - QML size: %dx%d, position: %d,%d\n", 
                    rect.width(), rect.height(), rect.x(), rect.y());
                
                emit viewportChanged(rect.x(), rect.y(), rect.width(), rect.height());

                // 通知 BGFX 视口变化
                // if (m_eventQueue)
                // {
                //     printf("BgfxItem sending size event: %dx%d\n", rect.width(), rect.height());
                //     // 发送自定义的视口变化事件
                //     // 注意：这里我们发送的是控件的大小，而不是窗口的大小
                //     // m_eventQueue->postSizeEvent(kDefaultWindowHandle, rect.width(), rect.height());
                // }
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

    // Static variables
    static QGuiApplication* s_app = nullptr;
    static QQmlApplicationEngine* s_engine = nullptr;
    static BgfxItem* s_bgfxItem = nullptr;
    static QWindow* s_window = nullptr;
    static QQuickWindow* s_quickWindow = nullptr;
    static EventQueue s_eventQueue;

    // 视口信息
    static struct ViewportInfo
    {
        int x = 0;
        int y = 0;
        int width = 0;
        int height = 0;
        bool valid = false;
    } s_viewport;

    static struct WindowParams
    {
        int32_t x = 100;
        int32_t y = 100;
        uint32_t width = 1280;
        uint32_t height = 720;
        const char* title = "BGFX";
    } s_windowParams;

    static void ensureWindowCreated()
    {
        if (!s_app)
        {
            static int argc = 1;
            static char appName[] = "bgfx-qt";
            static char* argv[] = { appName, nullptr };

            QGuiApplication::setAttribute(Qt::AA_DisableHighDpiScaling);

            s_app = new QGuiApplication(argc, argv);
            s_app->setApplicationName("BGFX-Qt");
        }

        if (!s_window)
        {
            // QML 模式
            if (!s_engine)
            {
                s_engine = new QQmlApplicationEngine();

                qmlRegisterType<BgfxItem>("BgfxModule", 1, 0, "BgfxItem");

                s_engine->load("qrc:qml/main.qml");
                if (!s_engine->rootObjects().isEmpty())
                {
                    s_quickWindow = qobject_cast<QQuickWindow*>(s_engine->rootObjects().first());
                    if (s_quickWindow)
                    {
                        s_window = s_quickWindow;
                        s_quickWindow->setTitle(QString::fromUtf8(s_windowParams.title));
                        s_quickWindow->setX(s_windowParams.x);
                        s_quickWindow->setY(s_windowParams.y);
                        s_quickWindow->resize(s_windowParams.width, s_windowParams.height);

                        s_bgfxItem = s_quickWindow->findChild<BgfxItem*>("bgfxRenderer");
                        if (s_bgfxItem)
                        {
                            s_bgfxItem->setEventQueue(&s_eventQueue);

                            // 连接视口变化信号
                            QObject::connect(s_bgfxItem, &BgfxItem::viewportChanged,
                                [](int x, int y, int width, int height) {
                                    s_viewport.x = x;
                                    s_viewport.y = y;
                                    s_viewport.width = width;
                                    s_viewport.height = height;
                                    s_viewport.valid = true;
                                });

                            // 初始更新视口
                            s_bgfxItem->updateViewport();
                        }
                    }
                }
            }
        }
    }

    // 获取当前视口信息
    void getViewport(int& x, int& y, int& width, int& height)
    {
        if (s_viewport.valid)
        {
            x = s_viewport.x;
            y = s_viewport.y;
            width = s_viewport.width;
            height = s_viewport.height;
            printf("getViewport returning: %d,%d %dx%d\n", x, y, width, height);
        }
        else
        {
            x = 0;
            y = 0;
            width = s_windowParams.width;
            height = s_windowParams.height;
            printf("getViewport returning default: %d,%d %dx%d\n", x, y, width, height);
        }
    }

    // Platform interface implementations
    const Event* poll()
    {
        if (s_app)
        {
            s_app->processEvents(QEventLoop::AllEvents, 0);
            s_app->sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }

        return s_eventQueue.poll();
    }

    const Event* poll(WindowHandle _handle)
    {
        if (s_app)
        {
            s_app->processEvents(QEventLoop::AllEvents, 0);
            s_app->sendPostedEvents(nullptr, QEvent::DeferredDelete);
        }

        return s_eventQueue.poll(_handle);
    }

    void release(const Event* _event)
    {
        s_eventQueue.release(_event);
    }

    WindowHandle createWindow(int32_t _x, int32_t _y, uint32_t _width, uint32_t _height, uint32_t _flags, const char* _title)
    {
        s_windowParams.x = _x;
        s_windowParams.y = _y;
        s_windowParams.width = _width;
        s_windowParams.height = _height;
        if (_title) s_windowParams.title = _title;

        ensureWindowCreated();

        return kDefaultWindowHandle;
    }

    void destroyWindow(WindowHandle _handle)
    {
        if (s_engine)
        {
            delete s_engine;
            s_engine = nullptr;
            s_bgfxItem = nullptr;
            s_quickWindow = nullptr;
        }
        s_window = nullptr;
    }

    void setWindowPos(WindowHandle _handle, int32_t _x, int32_t _y)
    {
        s_windowParams.x = _x;
        s_windowParams.y = _y;

        if (s_window)
        {
            s_window->setX(_x);
            s_window->setY(_y);
        }
    }

    void setWindowSize(WindowHandle _handle, uint32_t _width, uint32_t _height)
    {
        s_windowParams.width = _width;
        s_windowParams.height = _height;

        ensureWindowCreated();

        if (s_window)
        {
            s_window->resize(_width, _height);
        }
    }

    void setWindowTitle(WindowHandle _handle, const char* _title)
    {
        if (_title) s_windowParams.title = _title;

        ensureWindowCreated();

        if (s_window)
        {
            s_window->setTitle(QString::fromUtf8(_title));
        }
    }

    void setWindowFlags(WindowHandle _handle, uint32_t _flags, bool _enabled)
    {
        if (s_window)
        {
            Qt::WindowFlags qtFlags = s_window->flags();

            if (_flags & ENTRY_WINDOW_FLAG_FRAME)
            {
                if (_enabled)
                    qtFlags &= ~Qt::FramelessWindowHint;
                else
                    qtFlags |= Qt::FramelessWindowHint;
            }

            s_window->setFlags(qtFlags);
        }
    }

    void toggleFullscreen(WindowHandle _handle)
    {
        if (s_window)
        {
            if (s_window->visibility() == QWindow::FullScreen)
            {
                s_window->showNormal();
            }
            else
            {
                s_window->showFullScreen();
            }
        }
    }

    void setMouseLock(WindowHandle _handle, bool _lock)
    {
        if (s_bgfxItem)
        {
            if (_lock)
            {
                s_bgfxItem->setCursor(Qt::BlankCursor);
                s_bgfxItem->grabMouse();
            }
            else
            {
                s_bgfxItem->unsetCursor();
                s_bgfxItem->ungrabMouse();
            }
        }
        else if (s_window)
        {
            if (_lock)
                s_window->setCursor(Qt::BlankCursor);
            else
                s_window->unsetCursor();
        }
    }

    void* getNativeWindowHandle(WindowHandle _handle)
    {
        ensureWindowCreated();

        if (s_window)
        {
            s_window->create();
            return reinterpret_cast<void*>(s_window->winId());
        }

        return nullptr;
    }

    void* getNativeDisplayHandle()
    {
#if BX_PLATFORM_LINUX
        if (s_app && s_window)
        {
            QPlatformNativeInterface* native = s_app->platformNativeInterface();
            if (native)
            {
                return native->nativeResourceForWindow("display", s_window);
            }
        }
#endif
        return nullptr;
    }

} // namespace entry

#include "entry_qt.moc"
