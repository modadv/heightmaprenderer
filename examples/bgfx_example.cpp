#include <heightmaprenderer/heightmap_renderer.h>
#include <iostream>
#include <vector>
#include <cmath>

#ifdef HEIGHTMAP_WITH_BGFX
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

// 包含调试绘制功能
#include <bgfx/embedded_shader.h>

#ifdef _WIN32
#include <windows.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE || wParam == VK_SPACE) {
                PostQuitMessage(0);
                return 0;
            }
            break;
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

HWND createWindow(int width, int height) {
    WNDCLASSA wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = "HeightmapBGFXWindow";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClassA(&wc);
    
    HWND hwnd = CreateWindowExA(
        0,
        "HeightmapBGFXWindow",
        "Heightmap BGFX - Debug Render (ESC to exit)",
        WS_OVERLAPPEDWINDOW,
        100, 100, width + 16, height + 39,
        NULL, NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    return hwnd;
}
#endif

// 软件渲染器作为备用方案
class SoftwareRenderer {
private:
    std::vector<heightmap::Point3D> m_vertices;
    std::vector<unsigned int> m_indices;
    float m_time;
    int m_width, m_height;
    std::vector<uint32_t> m_framebuffer;

public:
    SoftwareRenderer() : m_time(0.0f), m_width(0), m_height(0) {}

    bool initialize(const heightmap::HeightmapRenderer& renderer, int width, int height) {
        m_width = width;
        m_height = height;
        m_framebuffer.resize(width * height);

        m_vertices = renderer.generateVertices();
        m_indices = renderer.generateIndices();

        if (m_vertices.empty()) {
            std::cerr << "No vertices to render\n";
            return false;
        }

        std::cout << "Software renderer initialized with " << m_vertices.size() 
                  << " vertices\n";
        return true;
    }

    void render(HDC hdc) {
        m_time += 0.016f;

        // 清空帧缓冲区
        std::fill(m_framebuffer.begin(), m_framebuffer.end(), 0x202020);

        // 简单的3D到2D投影
        float radius = 12.0f;
        float angle = m_time * 0.4f;
        
        // 相机位置
        float eye_x = radius * std::cos(angle);
        float eye_y = 6.0f;
        float eye_z = radius * std::sin(angle);

        // 找到高度范围用于颜色映射
        float minY = m_vertices[0].y;
        float maxY = m_vertices[0].y;
        for (const auto& v : m_vertices) {
            minY = std::min(minY, v.y);
            maxY = std::max(maxY, v.y);
        }

        // 渲染每个顶点为像素点
        for (const auto& vertex : m_vertices) {
            // 简单的透视投影
            float dx = vertex.x - eye_x;
            float dy = vertex.y - eye_y;
            float dz = vertex.z - eye_z;
            
            // 避免除零
            if (dz > -0.1f) continue;
            
            float perspective = -2.0f / dz;
            int screen_x = static_cast<int>(m_width/2 + dx * perspective * 200);
            int screen_y = static_cast<int>(m_height/2 - dy * perspective * 200);
            
            // 边界检查
            if (screen_x >= 0 && screen_x < m_width && 
                screen_y >= 0 && screen_y < m_height) {
                
                // 基于高度的颜色
                float normalizedHeight = (vertex.y - minY) / (maxY - minY);
                uint8_t r = static_cast<uint8_t>(255 * normalizedHeight);
                uint8_t g = static_cast<uint8_t>(255 * (1.0f - std::abs(normalizedHeight - 0.5f) * 2.0f));
                uint8_t b = static_cast<uint8_t>(255 * (1.0f - normalizedHeight));
                
                uint32_t color = 0xFF000000 | (r << 16) | (g << 8) | b;
                
                // 绘制3x3像素块使点更可见
                for (int dy = -1; dy <= 1; dy++) {
                    for (int dx = -1; dx <= 1; dx++) {
                        int px = screen_x + dx;
                        int py = screen_y + dy;
                        if (px >= 0 && px < m_width && py >= 0 && py < m_height) {
                            m_framebuffer[py * m_width + px] = color;
                        }
                    }
                }
            }
        }

        // 将帧缓冲区绘制到窗口
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = m_width;
        bmi.bmiHeader.biHeight = -m_height; // 负值表示自顶向下
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        SetDIBitsToDevice(hdc, 0, 0, m_width, m_height, 0, 0, 0, m_height,
                         m_framebuffer.data(), &bmi, DIB_RGB_COLORS);
    }
};

int runBGFXExample() {
    const int width = 1024;
    const int height = 768;

#ifdef _WIN32
    HWND hwnd = createWindow(width, height);
    if (!hwnd) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    std::cout << "BGFX failed to render geometry, falling back to software renderer\n";
    std::cout << "This will show a rotating point cloud of the heightmap\n";

    // 创建高度图
    heightmap::HeightmapRenderer renderer;
    
    std::vector<std::vector<float> > heightData;
    const int size = 32;
    heightData.resize(size);
    
    for (int y = 0; y < size; ++y) {
        heightData[y].resize(size);
        for (int x = 0; x < size; ++x) {
            float fx = (x - size/2.0f) * 0.3f;
            float fy = (y - size/2.0f) * 0.3f;
            
            float height = std::sin(fx) * std::cos(fy) * 2.0f;
            height += std::sin(fx * 3.0f) * std::cos(fy * 3.0f) * 0.5f;
            
            heightData[y][x] = height;
        }
    }

    if (!renderer.loadHeightmap(heightData)) {
        std::cerr << "Failed to load heightmap\n";
        return 1;
    }

    renderer.setScale(0.5f, 1.0f, 0.5f);

    // 创建软件渲染器
    SoftwareRenderer softRenderer;
    if (!softRenderer.initialize(renderer, width, height)) {
        std::cerr << "Failed to initialize software renderer\n";
        return 1;
    }

    std::cout << "Starting software render loop...\n";
    std::cout << "You should see a rotating 3D point cloud\n";
    std::cout << "Colors: Blue=Low, Green=Medium, Red=High\n";
    std::cout << "Press ESC to exit\n";

    // 主循环
    MSG msg = {};
    bool running = true;
    int frameCount = 0;
    
    while (running) {
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_QUIT) {
                running = false;
                break;
            }
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        
        if (running) {
            HDC hdc = GetDC(hwnd);
            if (hdc) {
                softRenderer.render(hdc);
                ReleaseDC(hwnd, hdc);
            }
            
            frameCount++;
            if (frameCount % 120 == 0) {
                std::cout << "Rendered " << frameCount << " frames...\n";
            }
            
            Sleep(16);
        }
    }

    return 0;

#else
    std::cerr << "This example requires Windows\n";
    return 1;
#endif
}

#endif // HEIGHTMAP_WITH_BGFX

int main() {
    std::cout << "=== HeightmapRenderer Software Fallback Example ===\n";
    std::cout << "Version: " << heightmap::HeightmapRenderer::getVersion() << "\n\n";

#ifdef HEIGHTMAP_WITH_BGFX
    try {
        return runBGFXExample();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception occurred\n";
        return 1;
    }
#else
    std::cout << "This example requires BGFX support.\n";
    return 0;
#endif
}