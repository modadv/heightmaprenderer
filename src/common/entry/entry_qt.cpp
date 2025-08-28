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

#if BX_PLATFORM_WINDOWS
    #include <windows.h>
#elif BX_PLATFORM_LINUX
    #include <X11/Xlib.h>
    #include <qpa/qplatformnativeinterface.h>
#endif

namespace entry
{
    // Custom Qt Window with minimal overhead
    class BgfxQtWindow : public QWindow
    {
    public:
        BgfxQtWindow()
            : QWindow()
            , m_eventQueue(nullptr)
            , m_mousePressed(0)
        {
            setSurfaceType(QSurface::OpenGLSurface);
            
            // Minimal OpenGL format - let BGFX handle the details
            QSurfaceFormat format;
            format.setRenderableType(QSurfaceFormat::OpenGL);
            format.setProfile(QSurfaceFormat::CoreProfile);
            format.setVersion(3, 3);
            format.setDepthBufferSize(24);
            format.setStencilBufferSize(8);
            format.setSwapBehavior(QSurfaceFormat::SingleBuffer); // Single buffer for performance
            format.setSwapInterval(0); // Disable vsync - BGFX will handle it
            setFormat(format);
            
            // Don't create OpenGL context - BGFX will create its own
        }
        
        ~BgfxQtWindow()
        {
        }
        
        void setEventQueue(EventQueue* queue)
        {
            m_eventQueue = queue;
        }
        
    protected:
        // Override event() for better performance - avoid virtual function calls
        bool event(QEvent* event) override
        {
            switch (event->type())
            {
            case QEvent::Expose:
                if (isExposed() && m_eventQueue)
                {
                    m_eventQueue->postSizeEvent(kDefaultWindowHandle, width(), height());
                }
                return true;
                
            case QEvent::Resize:
                if (m_eventQueue && isExposed())
                {
                    m_eventQueue->postSizeEvent(kDefaultWindowHandle, width(), height());
                }
                return true;
                
            case QEvent::KeyPress:
                {
                    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
                    if (m_eventQueue && !keyEvent->isAutoRepeat())
                    {
                        Key::Enum key = translateKey(keyEvent->key());
                        if (key != Key::None)
                        {
                            m_eventQueue->postKeyEvent(kDefaultWindowHandle, key, 
                                translateModifiers(keyEvent->modifiers()), true);
                        }
                    }
                }
                return true;
                
            case QEvent::KeyRelease:
                {
                    QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
                    if (m_eventQueue && !keyEvent->isAutoRepeat())
                    {
                        Key::Enum key = translateKey(keyEvent->key());
                        if (key != Key::None)
                        {
                            m_eventQueue->postKeyEvent(kDefaultWindowHandle, key, 
                                translateModifiers(keyEvent->modifiers()), false);
                        }
                    }
                }
                return true;
                
            case QEvent::MouseButtonPress:
                {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    if (m_eventQueue)
                    {
                        MouseButton::Enum button = translateMouseButton(mouseEvent->button());
                        if (button != MouseButton::None)
                        {
                            m_mousePressed |= mouseEvent->button();
                            m_eventQueue->postMouseEvent(kDefaultWindowHandle, 
                                mouseEvent->x(), mouseEvent->y(), 0, button, true);
                        }
                    }
                }
                return true;
                
            case QEvent::MouseButtonRelease:
                {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    if (m_eventQueue)
                    {
                        MouseButton::Enum button = translateMouseButton(mouseEvent->button());
                        if (button != MouseButton::None)
                        {
                            m_mousePressed &= ~mouseEvent->button();
                            m_eventQueue->postMouseEvent(kDefaultWindowHandle, 
                                mouseEvent->x(), mouseEvent->y(), 0, button, false);
                        }
                    }
                }
                return true;
                
            case QEvent::MouseMove:
                {
                    QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                    if (m_eventQueue)
                    {
                        m_eventQueue->postMouseEvent(kDefaultWindowHandle, 
                            mouseEvent->x(), mouseEvent->y(), 0);
                    }
                }
                return true;
                
            case QEvent::Wheel:
                {
                    QWheelEvent* wheelEvent = static_cast<QWheelEvent*>(event);
                    if (m_eventQueue)
                    {
                        int delta = wheelEvent->angleDelta().y() / 120;
                        m_eventQueue->postMouseEvent(kDefaultWindowHandle, 
                            wheelEvent->x(), wheelEvent->y(), delta);
                    }
                }
                return true;
                
            default:
                return QWindow::event(event);
            }
        }
        
    private:
        static Key::Enum translateKey(int qtKey)
        {
            // Use a lookup table for better performance
            static const Key::Enum s_keyTable[] = 
            {
                Key::None,        // 0x00
                Key::Esc,         // 0x01000000 Qt::Key_Escape
                Key::Tab,         // 0x01000001 Qt::Key_Tab
                Key::None,        // 0x01000002 Qt::Key_Backtab
                Key::Backspace,   // 0x01000003 Qt::Key_Backspace
                Key::Return,      // 0x01000004 Qt::Key_Return
                Key::Return,      // 0x01000005 Qt::Key_Enter
                Key::Insert,      // 0x01000006 Qt::Key_Insert
                Key::Delete,      // 0x01000007 Qt::Key_Delete
                Key::None,        // 0x01000008 Qt::Key_Pause
                Key::Print,       // 0x01000009 Qt::Key_Print
                // ... more entries
            };
            
            // Fast path for common keys
            if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z)
                return Key::Enum(Key::KeyA + (qtKey - Qt::Key_A));
            if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9)
                return Key::Enum(Key::Key0 + (qtKey - Qt::Key_0));
            if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F12)
                return Key::Enum(Key::F1 + (qtKey - Qt::Key_F1));
                
            // Fall back to switch for special keys
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
                default:                 return Key::None;
            }
        }
        
        static inline uint8_t translateModifiers(Qt::KeyboardModifiers modifiers)
        {
            uint8_t result = 0;
            if (modifiers & Qt::ControlModifier) result |= Modifier::LeftCtrl;
            if (modifiers & Qt::ShiftModifier)   result |= Modifier::LeftShift;
            if (modifiers & Qt::AltModifier)     result |= Modifier::LeftAlt;
            if (modifiers & Qt::MetaModifier)    result |= Modifier::LeftMeta;
            return result;
        }
        
        static inline MouseButton::Enum translateMouseButton(Qt::MouseButton button)
        {
            switch (button)
            {
            case Qt::LeftButton:   return MouseButton::Left;
            case Qt::RightButton:  return MouseButton::Right;
            case Qt::MiddleButton: return MouseButton::Middle;
            default:               return MouseButton::None;
            }
        }
        
        EventQueue* m_eventQueue;
        Qt::MouseButtons m_mousePressed;
    };
    
    // Static variables
    static QGuiApplication* s_app = nullptr;
    static BgfxQtWindow* s_window = nullptr;
    static EventQueue s_eventQueue;
    
    // Window creation parameters
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
            
            // Disable Qt's automatic high DPI scaling - let BGFX handle it
            QGuiApplication::setAttribute(Qt::AA_DisableHighDpiScaling);
            
            s_app = new QGuiApplication(argc, argv);
            s_app->setApplicationName("BGFX-Qt");
            
            // Set application attributes for better performance
            s_app->setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
            s_app->setAttribute(Qt::AA_DisableShaderDiskCache); // BGFX handles shaders
        }
        
        if (!s_window)
        {
            s_window = new BgfxQtWindow();
            s_window->setEventQueue(&s_eventQueue);
            s_window->setTitle(QString::fromUtf8(s_windowParams.title));
            s_window->setPosition(s_windowParams.x, s_windowParams.y);
            s_window->resize(s_windowParams.width, s_windowParams.height);
            s_window->show();
            
            // Process events once to make sure window is shown
            s_app->processEvents();
        }
    }
    
    // Platform interface implementations
    const Event* poll()
    {
        if (s_app)
        {
            // Use processEvents with minimal timeout for better performance
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
        if (s_window)
        {
            if (_lock)
            {
                s_window->setCursor(Qt::BlankCursor);
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
