// examples/04_test_visual.cpp
#include <terrain/terrain.h>
#include <cstdio>
#include <thread>
#include <chrono>
#include <cmath>

#ifdef _WIN32
#include <windows.h>

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) {
            PostQuitMessage(0);
        }
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
    
    HWND hWnd = CreateWindowExA(0, "TerrainTestWindow", "Terrain Visual Test (Press ESC to exit)",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        width, height, NULL, NULL, GetModuleHandle(NULL), NULL);
    
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    
    return hWnd;
}
#endif

// 简单的矩阵辅助函数
void CreateLookAt(float* result, 
                  float eyeX, float eyeY, float eyeZ,
                  float centerX, float centerY, float centerZ,
                  float upX, float upY, float upZ) {
    // 简化的lookAt实现
    float zx = eyeX - centerX;
    float zy = eyeY - centerY;
    float zz = eyeZ - centerZ;
    float len = sqrtf(zx*zx + zy*zy + zz*zz);
    zx /= len; zy /= len; zz /= len;
    
    float xx = upY * zz - upZ * zy;
    float xy = upZ * zx - upX * zz;
    float xz = upX * zy - upY * zx;
    len = sqrtf(xx*xx + xy*xy + xz*xz);
    xx /= len; xy /= len; xz /= len;
    
    float yx = zy * xz - zz * xy;
    float yy = zz * xx - zx * xz;
    float yz = zx * xy - zy * xx;
    
    result[0] = xx; result[1] = yx; result[2] = zx; result[3] = 0;
    result[4] = xy; result[5] = yy; result[6] = zy; result[7] = 0;
    result[8] = xz; result[9] = yz; result[10] = zz; result[11] = 0;
    result[12] = -(xx*eyeX + xy*eyeY + xz*eyeZ);
    result[13] = -(yx*eyeX + yy*eyeY + yz*eyeZ);
    result[14] = -(zx*eyeX + zy*eyeY + zz*eyeZ);
    result[15] = 1;
}

void CreatePerspective(float* result, float fovy, float aspect, float nearPlane, float farPlane) {
    float yScale = 1.0f / tanf(fovy * 0.5f * 3.14159f / 180.0f);
    float xScale = yScale / aspect;
    
    memset(result, 0, sizeof(float) * 16);
    result[0] = xScale;
    result[5] = yScale;
    result[10] = farPlane / (farPlane - nearPlane);
    result[11] = 1.0f;
    result[14] = -nearPlane * farPlane / (farPlane - nearPlane);
}

int main() {
    printf("=== Terrain Visual Test ===\n");
    printf("Press ESC to exit\n\n");
    
#ifdef _WIN32
    // 创建窗口
    HWND hwnd = CreateSimpleWindow(1024, 768);
    
    // 创建渲染器
    terrain::Renderer renderer;
    terrain::Config config;
    config.windowWidth = 1024;
    config.windowHeight = 768;
    
    printf("Initializing renderer...\n");
    if (!renderer.Initialize(&config)) {
        printf("Failed to initialize: %s\n", renderer.GetLastError());
        return -1;
    }
    
    // 设置渲染目标
    terrain::RenderTarget target;
    target.nativeHandle = hwnd;
    target.width = 1024;
    target.height = 768;
    target.type = terrain::RenderTarget::Window;
    
    renderer.SetRenderTarget(target);
    
    // 加载高度图
    printf("Loading heightmap...\n");
    if (!renderer.LoadHeightmap("textures/0049_16bit.png")) {
        printf("Failed to load heightmap: %s\n", renderer.GetLastError());
    }
    
    // 设置地形参数
    renderer.SetScale(200.0f, 50.0f);
    
    // 创建视图和投影矩阵
    float viewMatrix[16];
    float projMatrix[16];
    
    CreatePerspective(projMatrix, 60.0f, 1024.0f/768.0f, 0.1f, 1000.0f);
    
    // 主循环
    printf("Entering render loop...\n");
    float time = 0.0f;
    bool wireframe = false;
    
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        // 处理Windows消息
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_KEYDOWN) {
                if (msg.wParam == 'W') {
                    wireframe = !wireframe;
                    renderer.SetWireframe(wireframe);
                    printf("Wireframe: %s\n", wireframe ? "ON" : "OFF");
                }
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (msg.message == WM_QUIT) break;
        
        // 更新相机位置（围绕地形旋转）
        time += 0.016f;
        float camX = sinf(time * 0.5f) * 300.0f;
        float camZ = cosf(time * 0.5f) * 300.0f;
        float camY = 150.0f;
        
        CreateLookAt(viewMatrix, 
                     camX, camY, camZ,  // eye
                     0, 0, 0,           // center
                     0, 1, 0);          // up
        
        // 渲染
        renderer.BeginFrame();
        renderer.SetViewProjection(viewMatrix, projMatrix);
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