// entry_stubs.cpp
// 这个文件提供了BGFX示例框架中的一些辅助库（如 bgfx_utils, camera）
// 所依赖的最小函数实现，以解决链接错误。

#include "common/entry/entry_p.h"
#include <bx/file.h>
#include <bx/allocator.h>

namespace entry
{
    // 1. 提供一个默认的内存分配器
    static bx::DefaultAllocator s_allocator;
    bx::AllocatorI* getAllocator()
    {
        return &s_allocator;
    }

    // 2. 实现 tinystl 所需的分配器接口
    void* TinyStlAllocator::static_allocate(size_t _bytes)
    {
        return BX_ALLOC(getAllocator(), _bytes);
    }

    void TinyStlAllocator::static_deallocate(void* _ptr, size_t)
    {
        if (NULL != _ptr)
        {
            BX_FREE(getAllocator(), _ptr);
        }
    }

    // 3. 提供一个默认的文件读取器
    static bx::FileReader s_fileReader;
    bx::FileReaderI* getFileReader()
    {
        return &s_fileReader;
    }

    // 4. 为其他不再使用的函数提供空的“存根”实现，以满足链接器
    // --- 这些函数的功能已被Qt取代或在新架构中不再需要 ---

    const Event* poll() { return nullptr; }
    const Event* poll(WindowHandle) { return nullptr; }
    void release(const Event*) {}
    WindowHandle createWindow(int32_t, int32_t, uint32_t, uint32_t, uint32_t, const char*) { return {0}; }
    void destroyWindow(WindowHandle) {}
    void setWindowPos(WindowHandle, int32_t, int32_t) {}
    void setWindowSize(WindowHandle, uint32_t, uint32_t) {}
    void setWindowTitle(WindowHandle, const char*) {}
    void setWindowFlags(WindowHandle, uint32_t, bool) {}
    void toggleFullscreen(WindowHandle) {}
    void setMouseLock(WindowHandle, bool) {}
    void* getNativeWindowHandle(WindowHandle) { return nullptr; }
    void* getNativeDisplayHandle() { return nullptr; }
    void setCurrentDir(const char*) {}

    // AppI 相关的存根
    AppI::AppI(const char*, const char*, const char*) {}
    AppI::~AppI() {}
    const char* AppI::getName() const { return "BgfxItem"; }
    const char* AppI::getDescription() const { return "Bgfx QML Item"; }
    const char* AppI::getUrl() const { return ""; }
    AppI* AppI::getNext() { return nullptr; }
    AppI* getFirstApp() { return nullptr; }
    uint32_t getNumApps() { return 0; }

} // namespace entry

// bgfx_utils.cpp 中 load() 函数的依赖
bx::AllocatorI* g_allocator = entry::getAllocator();