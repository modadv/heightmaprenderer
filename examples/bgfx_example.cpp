#include <heightmaprenderer/heightmap_renderer.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <cstring>

#ifdef HEIGHTMAP_WITH_BGFX
#include <bgfx/bgfx.h>
#include <bgfx/platform.h>
#include <bx/math.h>

#ifdef _WIN32
#include <windows.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#elif defined(__linux__)
#define GLFW_EXPOSE_NATIVE_X11
#elif defined(__APPLE__)
#define GLFW_EXPOSE_NATIVE_COCOA
#endif

// Simple window system (using basic platform API)
#ifdef _WIN32
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        case WM_KEYDOWN:
            if (wParam == VK_ESCAPE) {
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
        "Heightmap BGFX Example",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        NULL, NULL,
        GetModuleHandle(NULL),
        NULL
    );
    
    ShowWindow(hwnd, SW_SHOW);
    return hwnd;
}
#endif

class BGFXHeightmapRenderer {
private:
    bgfx::VertexBufferHandle m_vbh;
    bgfx::IndexBufferHandle m_ibh;
    bgfx::ProgramHandle m_program;
    bool m_initialized;

    // Local vertex structure for BGFX
    struct BGFXVertex {
        float x, y, z;
        uint32_t color;
    };

public:
    BGFXHeightmapRenderer() : m_initialized(false) {
        m_vbh = BGFX_INVALID_HANDLE;
        m_ibh = BGFX_INVALID_HANDLE;
        m_program = BGFX_INVALID_HANDLE;
    }
    
    ~BGFXHeightmapRenderer() {
        if (m_initialized) {
            if (bgfx::isValid(m_vbh)) bgfx::destroy(m_vbh);
            if (bgfx::isValid(m_ibh)) bgfx::destroy(m_ibh);
            if (bgfx::isValid(m_program)) bgfx::destroy(m_program);
        }
    }

    bool initialize(const heightmap::HeightmapRenderer& renderer) {
        bgfx::VertexLayout layout;
        layout.begin()
            .add(bgfx::Attrib::Position, 3, bgfx::AttribType::Float)
            .add(bgfx::Attrib::Color0, 4, bgfx::AttribType::Uint8, true)
            .end();

        // Get vertex data - Use Point3D instead of Vertex
        std::vector<heightmap::Point3D> heightmapVertices = renderer.generateVertices();
        std::vector<unsigned int> indices = renderer.generateIndices();

        if (heightmapVertices.empty() || indices.empty()) {
            std::cerr << "Failed to get vertex or index data\n";
            return false;
        }

        // Convert heightmap vertices to BGFX vertices
        std::vector<BGFXVertex> bgfxVertices;
        bgfxVertices.reserve(heightmapVertices.size());

        for (size_t i = 0; i < heightmapVertices.size(); ++i) {
            const heightmap::Point3D& v = heightmapVertices[i];
            BGFXVertex vertex;
            vertex.x = v.x;
            vertex.y = v.y;
            vertex.z = v.z;
            
            // Generate color based on height (simple height mapping)
            float normalizedHeight = (v.y + 1.0f) / 3.0f; // Assume height range is -1 to 2
            normalizedHeight = (std::max)(0.0f, (std::min)(1.0f, normalizedHeight));
            
            uint8_t r = static_cast<uint8_t>(255 * normalizedHeight);
            uint8_t g = static_cast<uint8_t>(255 * (1.0f - normalizedHeight));
            uint8_t b = 128;
            vertex.color = 0xff000000 | (b << 16) | (g << 8) | r;
            
            bgfxVertices.push_back(vertex);
        }

        // Create vertex buffer
        const bgfx::Memory* vertexMem = bgfx::copy(bgfxVertices.data(), 
            static_cast<uint32_t>(bgfxVertices.size() * sizeof(BGFXVertex)));
        m_vbh = bgfx::createVertexBuffer(vertexMem, layout);

        // Create index buffer
        const bgfx::Memory* indexMem = bgfx::copy(indices.data(), 
            static_cast<uint32_t>(indices.size() * sizeof(uint32_t)));
        m_ibh = bgfx::createIndexBuffer(indexMem, BGFX_BUFFER_INDEX32);

        // Create simple shader program
        createShaders();

        m_initialized = true;
        return true;
    }

    void render() {
        if (!m_initialized) return;

        // Set view transform
        float view[16];
        float proj[16];
        
        bx::Vec3 eye = {0.0f, 10.0f, -10.0f};
        bx::Vec3 at = {0.0f, 0.0f, 0.0f};
        bx::Vec3 up = {0.0f, 1.0f, 0.0f};
        bx::mtxLookAt(view, eye, at, up);
        bx::mtxProj(proj, 60.0f, 16.0f/9.0f, 0.1f, 100.0f, bgfx::getCaps()->homogeneousDepth);
        
        bgfx::setViewTransform(0, view, proj);

        // Set model transform
        float mtx[16];
        bx::mtxIdentity(mtx);
        bgfx::setTransform(mtx);

        // Set vertex and index buffers
        bgfx::setVertexBuffer(0, m_vbh);
        bgfx::setIndexBuffer(m_ibh);

        // Submit draw call
        bgfx::submit(0, m_program);
    }

private:
    void createShaders() {
        // Simple embedded shaders
        const char* vs_source = 
            "$input a_position, a_color0\n"
            "$output v_color0\n"
            "\n"
            "#include <bgfx_shader.sh>\n"
            "\n"
            "void main() {\n"
            "    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));\n"
            "    v_color0 = a_color0;\n"
            "}\n";

        const char* fs_source = 
            "$input v_color0\n"
            "\n"
            "#include <bgfx_shader.sh>\n"
            "\n"
            "void main() {\n"
            "    gl_FragColor = v_color0;\n"
            "}\n";

        // Note: In real applications, shaders need to be pre-compiled
        bgfx::ShaderHandle vs = bgfx::createShader(bgfx::copy(vs_source, strlen(vs_source) + 1));
        bgfx::ShaderHandle fs = bgfx::createShader(bgfx::copy(fs_source, strlen(fs_source) + 1));
        m_program = bgfx::createProgram(vs, fs, true);
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
#else
    std::cerr << "This example currently only supports Windows platform\n";
    return 1;
#endif

    // Initialize bgfx
    bgfx::PlatformData pd;
    memset(&pd, 0, sizeof(pd));
#ifdef _WIN32
    pd.nwh = hwnd;
#endif

    bgfx::setPlatformData(pd);

    bgfx::Init init;
    memset(&init, 0, sizeof(init));
    init.type = bgfx::RendererType::Count; // Auto-select
    init.resolution.width = width;
    init.resolution.height = height;
    init.resolution.reset = BGFX_RESET_VSYNC;

    if (!bgfx::init(init)) {
        std::cerr << "BGFX initialization failed\n";
        return 1;
    }

    std::cout << "BGFX initialized successfully\n";
    std::cout << "Renderer: " << bgfx::getRendererName(bgfx::getRendererType()) << "\n";

    // Set view
    bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
    bgfx::setViewRect(0, 0, 0, uint16_t(width), uint16_t(height));

    // Create heightmap renderer
    heightmap::HeightmapRenderer renderer;
    
    // Create test heightmap data
    std::vector<std::vector<float> > heightData;
    const int size = 50;
    heightData.resize(size);
    for (int y = 0; y < size; ++y) {
        heightData[y].resize(size);
        for (int x = 0; x < size; ++x) {
            float fx = (x - size/2) * 0.2f;
            float fy = (y - size/2) * 0.2f;
            heightData[y][x] = std::sin(fx) * std::cos(fy) * 2.0f;
        }
    }

    if (!renderer.loadHeightmap(heightData)) {
        std::cerr << "Failed to load heightmap\n";
        bgfx::shutdown();
        return 1;
    }

    renderer.setScale(0.5f, 1.0f, 0.5f);

    // Create BGFX renderer
    BGFXHeightmapRenderer bgfxRenderer;
    if (!bgfxRenderer.initialize(renderer)) {
        std::cerr << "Failed to initialize BGFX renderer\n";
        bgfx::shutdown();
        return 1;
    }

    std::cout << "Starting render loop... (Press ESC to exit)\n";

    // Main loop
#ifdef _WIN32
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            // Render
            bgfxRenderer.render();
            bgfx::frame();
        }
    }
#endif

    bgfx::shutdown();
    return 0;
}

#endif // HEIGHTMAP_WITH_BGFX

int main() {
    std::cout << "=== HeightmapRenderer BGFX Example ===\n";
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
    std::cout << "Please rebuild the project with -DHEIGHTMAP_WITH_BGFX=ON.\n";
    return 0;
#endif
}