#include "entry_p.h"

#include <bgfx/platform.h>
#include <bgfx/bgfx.h>
#include <bx/thread.h>
#include <bx/os.h>

#include <QGuiApplication>
#include <QWindow>
#include <QOpenGLContext>
#include <QTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QTouchEvent>
#include <QDir>

#if BX_PLATFORM_WINDOWS
    #include <windows.h>
#elif BX_PLATFORM_LINUX
    #include <X11/Xlib.h>
    #include <qpa/qplatformnativeinterface.h>
#elif BX_PLATFORM_OSX
    #include <Cocoa/Cocoa.h>
#endif

// Optional debug console for development
#define ENTRY_QT_DEBUG_CONSOLE 0

#if ENTRY_QT_DEBUG_CONSOLE && BX_PLATFORM_WINDOWS
#include <io.h>
#include <fcntl.h>
#include <iostream>

static void CreateConsoleWindow()
{
    AllocConsole();
    
    FILE* pCout;
    freopen_s(&pCout, "CONOUT$", "w", stdout);
    
    FILE* pCerr;
    freopen_s(&pCerr, "CONOUT$", "w", stderr);
    
    FILE* pCin;
    freopen_s(&pCin, "CONIN$", "r", stdin);
    
    std::ios::sync_with_stdio();
    
    SetConsoleTitleA("Debug Console");
}
#endif

namespace entry
{
    // Custom Qt Window with OpenGL support for BGFX
    class BgfxQtWindow : public QWindow
    {
    public:
        BgfxQtWindow()
            : QWindow()
            , m_context(nullptr)
            , m_eventQueue(nullptr)
            , m_frameTimer(nullptr)
        {
            setSurfaceType(QSurface::OpenGLSurface);
            
            // Setup OpenGL format for BGFX
            QSurfaceFormat format;
            format.setRenderableType(QSurfaceFormat::OpenGL);
            format.setProfile(QSurfaceFormat::CoreProfile);
            format.setVersion(3, 3);
            format.setDepthBufferSize(24);
            format.setStencilBufferSize(8);
            format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
            format.setSwapInterval(0); // Disable vsync, let BGFX handle it
            setFormat(format);
            
            // Create OpenGL context
            m_context = new QOpenGLContext(this);
            m_context->setFormat(format);
            m_context->create();
            
            // Setup frame timer for continuous rendering
            m_frameTimer = new QTimer(this);
            m_frameTimer->setInterval(0); // As fast as possible
            connect(m_frameTimer, &QTimer::timeout, [this]() {
                if (isExposed())
                {
                    requestUpdate();
                }
            });
            m_frameTimer->start();
        }
        
        ~BgfxQtWindow()
        {
            if (m_frameTimer)
            {
                m_frameTimer->stop();
                delete m_frameTimer;
            }
            delete m_context;
        }
        
        void setEventQueue(EventQueue* queue)
        {
            m_eventQueue = queue;
        }
        
        void makeCurrent()
        {
            if (m_context && isExposed())
            {
                m_context->makeCurrent(this);
            }
        }
        
        void swapBuffers()
        {
            if (m_context && isExposed())
            {
                m_context->swapBuffers(this);
            }
        }
        
    protected:
        bool event(QEvent* event) override
        {
            switch (event->type())
            {
            case QEvent::UpdateRequest:
                // Just process the update request, don't post empty event
                // The main loop will handle polling
                return true;
                
            default:
                return QWindow::event(event);
            }
        }
        
        void exposeEvent(QExposeEvent* event) override
        {
            Q_UNUSED(event);
            
            if (isExposed() && m_eventQueue)
            {
                m_eventQueue->postSizeEvent(kDefaultWindowHandle, width(), height());
            }
        }
        
        void resizeEvent(QResizeEvent* event) override
        {
            Q_UNUSED(event);
            
            if (m_eventQueue && isExposed())
            {
                m_eventQueue->postSizeEvent(kDefaultWindowHandle, width(), height());
            }
        }
        
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
                
                // Handle text input
                if (!event->text().isEmpty())
                {
                    const QString text = event->text();
                    for (int i = 0; i < text.length(); ++i)
                    {
                        uint8_t utf8[4] = {};
                        QChar ch = text.at(i);
                        if (ch.unicode() < 0x80)
                        {
                            utf8[0] = ch.unicode();
                            m_eventQueue->postCharEvent(kDefaultWindowHandle, 1, utf8);
                        }
                    }
                }
            }
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
        }
        
        void mousePressEvent(QMouseEvent* event) override
        {
            if (m_eventQueue)
            {
                MouseButton::Enum button = translateMouseButton(event->button());
                if (button != MouseButton::None)
                {
                    m_eventQueue->postMouseEvent(kDefaultWindowHandle, 
                        event->x(), event->y(), 0, button, true);
                }
            }
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
        }
        
        void mouseMoveEvent(QMouseEvent* event) override
        {
            if (m_eventQueue)
            {
                m_eventQueue->postMouseEvent(kDefaultWindowHandle, 
                    event->x(), event->y(), 0);
            }
        }
        
        void wheelEvent(QWheelEvent* event) override
        {
            if (m_eventQueue)
            {
                int delta = event->angleDelta().y() / 120;
                m_eventQueue->postMouseEvent(kDefaultWindowHandle, 
                    event->x(), event->y(), delta);
            }
        }
        
    private:
        static Key::Enum translateKey(int qtKey)
        {
            switch(qtKey)
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
                // Qt::Key_Equal doesn't exist, use Qt::Key_Equal if available
                // or handle '=' as a character input instead
                case Qt::Key_Comma:      return Key::Comma;
                case Qt::Key_Period:     return Key::Period;
                case Qt::Key_Slash:      return Key::Slash;
                case Qt::Key_Semicolon:  return Key::Semicolon;
                case Qt::Key_BracketLeft: return Key::LeftBracket;
                case Qt::Key_BracketRight: return Key::RightBracket;
                case Qt::Key_Backslash:  return Key::Backslash;
                case Qt::Key_Apostrophe: return Key::Quote;
                case Qt::Key_F1:         return Key::F1;
                case Qt::Key_F2:         return Key::F2;
                case Qt::Key_F3:         return Key::F3;
                case Qt::Key_F4:         return Key::F4;
                case Qt::Key_F5:         return Key::F5;
                case Qt::Key_F6:         return Key::F6;
                case Qt::Key_F7:         return Key::F7;
                case Qt::Key_F8:         return Key::F8;
                case Qt::Key_F9:         return Key::F9;
                case Qt::Key_F10:        return Key::F10;
                case Qt::Key_F11:        return Key::F11;
                case Qt::Key_F12:        return Key::F12;
                case Qt::Key_0:          return Key::Key0;
                case Qt::Key_1:          return Key::Key1;
                case Qt::Key_2:          return Key::Key2;
                case Qt::Key_3:          return Key::Key3;
                case Qt::Key_4:          return Key::Key4;
                case Qt::Key_5:          return Key::Key5;
                case Qt::Key_6:          return Key::Key6;
                case Qt::Key_7:          return Key::Key7;
                case Qt::Key_8:          return Key::Key8;
                case Qt::Key_9:          return Key::Key9;
                case Qt::Key_A:          return Key::KeyA;
                case Qt::Key_B:          return Key::KeyB;
                case Qt::Key_C:          return Key::KeyC;
                case Qt::Key_D:          return Key::KeyD;
                case Qt::Key_E:          return Key::KeyE;
                case Qt::Key_F:          return Key::KeyF;
                case Qt::Key_G:          return Key::KeyG;
                case Qt::Key_H:          return Key::KeyH;
                case Qt::Key_I:          return Key::KeyI;
                case Qt::Key_J:          return Key::KeyJ;
                case Qt::Key_K:          return Key::KeyK;
                case Qt::Key_L:          return Key::KeyL;
                case Qt::Key_M:          return Key::KeyM;
                case Qt::Key_N:          return Key::KeyN;
                case Qt::Key_O:          return Key::KeyO;
                case Qt::Key_P:          return Key::KeyP;
                case Qt::Key_Q:          return Key::KeyQ;
                case Qt::Key_R:          return Key::KeyR;
                case Qt::Key_S:          return Key::KeyS;
                case Qt::Key_T:          return Key::KeyT;
                case Qt::Key_U:          return Key::KeyU;
                case Qt::Key_V:          return Key::KeyV;
                case Qt::Key_W:          return Key::KeyW;
                case Qt::Key_X:          return Key::KeyX;
                case Qt::Key_Y:          return Key::KeyY;
                case Qt::Key_Z:          return Key::KeyZ;
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
        
        QOpenGLContext* m_context;
        EventQueue* m_eventQueue;
        QTimer* m_frameTimer;
    };
    
    // Static variables
    static QGuiApplication* s_app = nullptr;
    static BgfxQtWindow* s_window = nullptr;
    static EventQueue s_eventQueue;
    
    // Window creation parameters (cached for delayed creation)
    static struct WindowParams
    {
        int32_t x = 100;
        int32_t y = 100;
        uint32_t width = 1280;
        uint32_t height = 720;
        const char* title = "BGFX";
    } s_windowParams;
    
    // Helper function to ensure window exists
    static void ensureWindowCreated()
    {
        if (!s_app)
        {
            static int argc = 1;
            static char appName[] = "bgfx-qt";
            static char* argv[] = { appName, nullptr };
            
            s_app = new QGuiApplication(argc, argv);
            s_app->setApplicationName("BGFX-Qt");
            s_app->setApplicationDisplayName(QString::fromUtf8(s_windowParams.title));
        }
        
        if (!s_window)
        {
            s_window = new BgfxQtWindow();
            s_window->setEventQueue(&s_eventQueue);
            s_window->setTitle(QString::fromUtf8(s_windowParams.title));
            s_window->setPosition(s_windowParams.x, s_windowParams.y);
            s_window->resize(s_windowParams.width, s_windowParams.height);
            s_window->show();
            
            // Process events to make sure window is shown
            s_app->processEvents();
        }
    }
    
    // Platform interface implementations
    const Event* poll()
    {
        if (s_app)
        {
            s_app->processEvents(QEventLoop::AllEvents, 0);
        }
        
        return s_eventQueue.poll();
    }
    
    const Event* poll(WindowHandle _handle)
    {
        if (s_app)
        {
            s_app->processEvents(QEventLoop::AllEvents, 0);
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
        if (s_window)
        {
            s_window->destroy();
            delete s_window;
            s_window = nullptr;
        }
    }
    
    void setWindowPos(WindowHandle _handle, int32_t _x, int32_t _y)
    {
        s_windowParams.x = _x;
        s_windowParams.y = _y;
        
        if (s_window)
        {
            s_window->setPosition(_x, _y);
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
        
        if (s_app)
        {
            s_app->setApplicationDisplayName(QString::fromUtf8(_title));
        }
    }
    
    void setWindowFlags(WindowHandle _handle, uint32_t _flags, bool _enabled)
    {
        if (s_window)
        {
            Qt::WindowFlags qtFlags = s_window->flags();
            
            if (_flags & ENTRY_WINDOW_FLAG_ASPECT_RATIO)
            {
                // TODO: Implement aspect ratio constraint
            }
            
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
        if (s_window)
        {
            if (_lock)
            {
                s_window->setCursor(Qt::BlankCursor);
                // TODO: Implement proper mouse lock/capture
            }
            else
            {
                s_window->unsetCursor();
            }
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

int main(int _argc, char** _argv)
{
#if ENTRY_QT_DEBUG_CONSOLE && BX_PLATFORM_WINDOWS
    CreateConsoleWindow();
#endif
    
    int result = entry::main(_argc, const_cast<const char**>(_argv));
    
    // Cleanup
    if (entry::s_window)
    {
        delete entry::s_window;
        entry::s_window = nullptr;
    }
    
    if (entry::s_app)
    {
        delete entry::s_app;
        entry::s_app = nullptr;
    }
    
    return result;
}
