// examples/03_test_heightmap.cpp
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
    
    HWND hWnd = CreateWindowExA(0, "TerrainTestWindow", "Heightmap Load Test",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        width, height, NULL, NULL, GetModuleHandle(NULL), NULL);
    
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    
    return hWnd;
}
#endif

int main() {
    printf("=== Terrain Heightmap Loading Test ===\n");
    
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
    
    // 测试加载高度图
    printf("\n=== Testing Heightmap Loading ===\n");
    
    // 测试1：加载存在的文件
    printf("\nTest 1: Loading textures/0049_16bit.png\n");
    if (renderer.LoadHeightmap("textures/0049_16bit.png")) {
        printf("SUCCESS: Heightmap loaded\n");
    } else {
        printf("FAILED: %s\n", renderer.GetLastError());
    }
    
    // 测试2：加载另一个文件
    printf("\nTest 2: Loading textures/1972_16bit.png\n");
    if (renderer.LoadHeightmap("textures/1972_16bit.png")) {
        printf("SUCCESS: Heightmap loaded\n");
    } else {
        printf("FAILED: %s\n", renderer.GetLastError());
    }
    
    // 测试3：加载不存在的文件
    printf("\nTest 3: Loading non-existent file\n");
    if (renderer.LoadHeightmap("notexist.png")) {
        printf("SUCCESS: Heightmap loaded\n");
    } else {
        printf("EXPECTED FAILURE: %s\n", renderer.GetLastError());
    }
    
    // 渲染几帧看看
    printf("\nRendering a few frames...\n");
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
    
    printf("\nTest completed!\n");
    return 0;
}