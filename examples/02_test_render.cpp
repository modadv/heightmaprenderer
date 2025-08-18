// examples/test_render.cpp
#include <terrain/terrain.h>
#include <cstdio>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

HWND CreateSimpleWindow(int width, int height) {
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = "TerrainTestWindow";
    
    RegisterClassExA(&wc);
    
    HWND hWnd = CreateWindowExA(0, "TerrainTestWindow", "Terrain Renderer Test",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        width, height, NULL, NULL, GetModuleHandle(NULL), NULL);
    
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    
    return hWnd;
}
#endif

int main() {
    printf("=== Terrain Renderer Basic Test ===\n");
    
#ifdef _WIN32
    // 创建窗口
    HWND hwnd = CreateSimpleWindow(800, 600);
    
    // 创建渲染器
    terrain::Renderer renderer;
    terrain::Config config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    
    printf("Initializing renderer...\n");
    if (!renderer.Initialize(&config)) {
        printf("Failed to initialize: %s\n", renderer.GetLastError());
        return -1;
    }
    
    // 设置渲染目标
    terrain::RenderTarget target;
    target.nativeHandle = hwnd;
    target.width = 800;
    target.height = 600;
    target.type = terrain::RenderTarget::Window;
    
    renderer.SetRenderTarget(target);
    
    printf("Rendering a few frames...\n");
    
    // 渲染几帧测试
    for (int i = 0; i < 60; i++) {
        MSG msg = {};
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        renderer.BeginFrame();
        renderer.Draw();
        renderer.EndFrame();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
    
    printf("Shutting down...\n");
    renderer.Shutdown();
    
    DestroyWindow(hwnd);
#else
    printf("This test requires Windows platform\n");
#endif
    
    printf("Test completed successfully!\n");
    return 0;
}