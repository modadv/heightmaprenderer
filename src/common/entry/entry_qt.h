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
#include <memory>

namespace entry {

    // Qt QML specific configuration
    struct QtConfig {
        bool useOpenGL = true;
        bool vsync = true;
        int msaaLevel = 0;
        bgfx::RendererType::Enum renderer = bgfx::RendererType::Count; // auto-select
    };

    // Create QML render window
    QQuickWindow* createQmlRenderWindow(const QtConfig& config = {});

    // Set QML window as bgfx render target
    bool setQmlWindowHandle(QQuickWindow* window);

    // Get native handle from QML window
    void* getQmlNativeWindowHandle(QQuickWindow* window);
}
