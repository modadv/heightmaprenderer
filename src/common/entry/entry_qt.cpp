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
#include <QDir>
#include <QDebug>
#include <stdio.h>

#if BX_PLATFORM_WINDOWS
#include <windows.h>
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
    
    SetConsoleTitleA("BGFX-Qt Debug Console");
    
    printf("=== Debug Console Created ===\n");
}
#endif

namespace entry
{
    // Custom Qt Window with OpenGL support
    class BgfxQtWindow : public QWindow
    {
    public:
        BgfxQtWindow()
            : QWindow()
            , m_context(nullptr)
            , m_eventQueue(nullptr)
            , m_update(true)
        {
            printf("BgfxQtWindow constructor\n");
            
            setSurfaceType(QSurface::OpenGLSurface);
            
            QSurfaceFormat format;
            format.setRenderableType(QSurfaceFormat::OpenGL);
            format.setProfile(QSurfaceFormat::CoreProfile);
            format.setVersion(3, 3);
            format.setDepthBufferSize(24);
            format.setStencilBufferSize(8);
            format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
            setFormat(format);
            
            m_context = new QOpenGLContext(this);
            m_context->setFormat(format);
            if (!m_context->create())
            {
                printf("Failed to create OpenGL context\n");
            }
            else
            {
                printf("OpenGL context created successfully\n");
            }
        }
        
        ~BgfxQtWindow()
        {
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
        void exposeEvent(QExposeEvent* event) override
        {
            Q_UNUSED(event);
            printf("Window expose event: %s\n", isExposed() ? "exposed" : "hidden");
            
            if (isExposed() && m_eventQueue)
            {
                m_eventQueue->postSizeEvent(kDefaultWindowHandle, width(), height());
            }
        }
        
        void resizeEvent(QResizeEvent* event) override
        {
            Q_UNUSED(event);
            printf("Window resize: %dx%d\n", width(), height());
            
            if (m_eventQueue && isExposed())
            {
                m_eventQueue->postSizeEvent(kDefaultWindowHandle, width(), height());
            }
        }
        
        void keyPressEvent(QKeyEvent* event) override
        {
            if (m_eventQueue)
            {
                Key::Enum key = translateKey(event->key());
                if (key != Key::None)
                {
                    m_eventQueue->postKeyEvent(kDefaultWindowHandle, key, 
                        translateModifiers(event->modifiers()), true);
                }
            }
        }
        
        void keyReleaseEvent(QKeyEvent* event) override
        {
            if (m_eventQueue)
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
                m_eventQueue->postMouseEvent(kDefaultWindowHandle, 
                    event->x(), event->y(), event->angleDelta().y() / 120);
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
                case Qt::Key_PageUp:     return Key::PageUp;
                case Qt::Key_PageDown:   return Key::PageDown;
                case Qt::Key_Home:       return Key::Home;
                case Qt::Key_End:        return Key::End;
                case Qt::Key_Print:      return Key::Print;
                case Qt::Key_Plus:       return Key::Plus;
                case Qt::Key_Minus:      return Key::Minus;
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
        bool m_update;
    };
    
    // Static variables
    static QGuiApplication* s_app = nullptr;
    static BgfxQtWindow* s_window = nullptr;
    static EventQueue s_eventQueue;
    static bool s_exit = false;
    static int32_t s_windowPosX = 100;
    static int32_t s_windowPosY = 100;
    static uint32_t s_windowWidth = 1280;
    static uint32_t s_windowHeight = 720;
    static const char* s_windowTitle = "BGFX";
    
    // Helper function to ensure window exists
    static void ensureWindowCreated()
    {
        if (!s_app)
        {
            printf("Creating QGuiApplication in ensureWindowCreated...\n");
            static int argc = 1;
            static char appName[] = "bgfx-qt";
            static char* argv[] = { appName, nullptr };
            
            s_app = new QGuiApplication(argc, argv);
            s_app->setApplicationName("BGFX-Qt");
        }
        
        if (!s_window)
        {
            printf("Creating BgfxQtWindow in ensureWindowCreated...\n");
            s_window = new BgfxQtWindow();
            s_window->setEventQueue(&s_eventQueue);
            s_window->setTitle(QString::fromUtf8(s_windowTitle));
            s_window->setPosition(s_windowPosX, s_windowPosY);
            s_window->resize(s_windowWidth, s_windowHeight);
            s_window->show();
            
            // Process events to make sure window is shown
            s_app->processEvents();
            
            printf("Window created with title: %s, size: %dx%d\n", 
                   s_windowTitle, s_windowWidth, s_windowHeight);
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
        printf("createWindow: %dx%d at (%d,%d), title: %s\n", _width, _height, _x, _y, _title);
        
        s_windowPosX = _x;
        s_windowPosY = _y;
        s_windowWidth = _width;
        s_windowHeight = _height;
        if (_title) s_windowTitle = _title;
        
        ensureWindowCreated();
        
        return kDefaultWindowHandle;
    }
    
    void destroyWindow(WindowHandle _handle)
    {
        printf("destroyWindow\n");
        
        if (s_window)
        {
            s_window->destroy();
            delete s_window;
            s_window = nullptr;
        }
        
        s_exit = true;
    }
    
    void setWindowPos(WindowHandle _handle, int32_t _x, int32_t _y)
    {
        printf("setWindowPos: %d, %d\n", _x, _y);
        s_windowPosX = _x;
        s_windowPosY = _y;
        
        if (s_window)
        {
            s_window->setPosition(_x, _y);
        }
    }
    
    void setWindowSize(WindowHandle _handle, uint32_t _width, uint32_t _height)
    {
        printf("setWindowSize: %dx%d\n", _width, _height);
        s_windowWidth = _width;
        s_windowHeight = _height;
        
        // 如果窗口还不存在，在这里创建它
        ensureWindowCreated();
        
        if (s_window)
        {
            s_window->resize(_width, _height);
        }
    }
    
    void setWindowTitle(WindowHandle _handle, const char* _title)
    {
        printf("setWindowTitle: %s\n", _title);
        if (_title) s_windowTitle = _title;
        
        // 如果窗口还不存在，在这里创建它
        ensureWindowCreated();
        
        if (s_window)
        {
            s_window->setTitle(QString::fromUtf8(_title));
        }
    }
    
    void setWindowFlags(WindowHandle _handle, uint32_t _flags, bool _enabled)
    {
        // TODO
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
        printf("getNativeWindowHandle called\n");
        
        // 确保窗口已创建
        ensureWindowCreated();
        
        if (s_window)
        {
            s_window->create();
            WId wid = s_window->winId();
            printf("Window ID: %p\n", reinterpret_cast<void*>(wid));
            return reinterpret_cast<void*>(wid);
        }
        
        printf("getNativeWindowHandle: window still null after ensureWindowCreated!\n");
        return nullptr;
    }
    
    void* getNativeDisplayHandle()
    {
        printf("getNativeDisplayHandle\n");
        
#if BX_PLATFORM_LINUX
        if (s_app && s_window)
        {
            QPlatformNativeInterface* native = s_app->platformNativeInterface();
            if (native)
            {
                void* display = native->nativeResourceForWindow("display", s_window);
                printf("Native display: %p\n", display);
                return display;
            }
        }
#endif
        return nullptr;
    }
    
} // namespace entry

int main(int _argc, char** _argv)
{
#if BX_PLATFORM_WINDOWS
    CreateConsoleWindow();
#endif
    
    printf("=== Qt Entry Point ===\n");
    
    char cwd[1024];
    if (GetCurrentDirectoryA(sizeof(cwd), cwd))
    {
        printf("Working directory: %s\n", cwd);
    }
    
    printf("argc: %d\n", _argc);
    for (int i = 0; i < _argc; i++) {
        printf("argv[%d]: %s\n", i, _argv[i]);
    }
    
    printf("Calling entry::main...\n");
    int result = entry::main(_argc, const_cast<const char**>(_argv));
    
    printf("=== Exiting with result: %d ===\n", result);
    
#if BX_PLATFORM_WINDOWS
    printf("\nPress Enter to close console...\n");
    getchar();
#endif
    
    if (entry::s_app)
    {
        delete entry::s_app;
        entry::s_app = nullptr;
    }
    
    return result;
}
