#pragma once
#include "heightmap/heightmap_renderer.h"
#include "common/common.h"
#include "common/camera.h"
#include "common/imgui/imgui.h"
#include "common/bgfx_utils.h"

#include <stdio.h>
#include <cstdio>


namespace entry { void getViewport(int& x, int& y, int& w, int& h); }

class HeightmapApp : public entry::AppI {
public:
    HeightmapApp(const char* name, const char* description, const char* url)
        : entry::AppI(name, description, url) {}

    // 添加虚析构函数实现
    virtual ~HeightmapApp() override = default;

    void init(int32_t argc, const char* const* argv, uint32_t width, uint32_t height) override {
        Args args(argc, argv);
        
        m_width = width;
        m_height = height;
        m_debug = BGFX_DEBUG_NONE;
        m_reset = BGFX_RESET_NONE;

        printf("HeightmapApp::init() called with %dx%d\n", width, height);

        // Initialize bgfx
        bgfx::Init init;
        init.type = args.m_type;
        init.vendorId = args.m_pciId;
        init.resolution.width = m_width;
        init.resolution.height = m_height;
        init.resolution.reset = m_reset;
        init.platformData.nwh = entry::getNativeWindowHandle(entry::kDefaultWindowHandle);
        init.platformData.ndt = entry::getNativeDisplayHandle();
        
        bgfx::init(init);
        bgfx::setDebug(m_debug);
        bgfx::setViewClear(0, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);
        bgfx::setViewClear(1, BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH, 0x303030ff, 1.0f, 0);

        // Initialize subsystems
        imguiCreate();
        cameraCreate();
        cameraSetPosition({0.0f, 0.9f, -1.3f});
        cameraSetVerticalAngle(0);

        // Initialize heightmap renderer
        m_heightmapRenderer.init(m_width, m_height);
        
        m_timeOffset = bx::getHPCounter();
        
        printf("HeightmapApp::init() completed\n");
    }

    // 添加shutdown方法实现
    int shutdown() override {
        printf("HeightmapApp::shutdown() called\n");
        m_heightmapRenderer.shutdown();
        cameraDestroy();
        imguiDestroy();
        bgfx::shutdown();
        return 0;
    }


    bool update() override {
        int vx, vy, vw, vh;
        entry::getViewport(vx, vy, vw, vh);
        m_heightmapRenderer.setViewport(vx, vy, vw, vh);

        uint32_t oldWidth = m_width;
        uint32_t oldHeight = m_height;
        
        if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState)) {

            int64_t now = bx::getHPCounter();
            static int64_t last = now;
            const int64_t frameTime = now - last;
            last = now;
            const double freq = double(bx::getHPFrequency());
            const float deltaTime = float(frameTime / freq);

            // 添加调试信息
            static int frameCount = 0;
            frameCount++;
            if (frameCount % 60 == 0) {
                printf("HeightmapApp::update() frame %d, window size: %dx%d\n",
                    frameCount, m_width, m_height);
            }

            // *** 关键修改：检查processEvents是否改变了尺寸 ***
            if (m_width != oldWidth || m_height != oldHeight) {
                printf("HeightmapApp processEvents changed size from %dx%d to %dx%d\n",
                    oldWidth, oldHeight, m_width, m_height);
                    
                // 立即更新渲染器尺寸
                m_heightmapRenderer.updateSize(m_width, m_height);
                
                // BGFX后台缓冲区已经在processEvents中被重置了
            } else {
                // 每次都调用updateSize确保同步（临时调试）
                if (frameCount % 60 == 0) {
                    printf("Forcing renderer size update to %dx%d\n", m_width, m_height);
                    m_heightmapRenderer.updateSize(m_width, m_height);
                }
            }

            // Begin ImGui frame
            imguiBeginFrame(
                m_mouseState.m_mx, m_mouseState.m_my,
                (m_mouseState.m_buttons[entry::MouseButton::Left] ? IMGUI_MBUT_LEFT : 0) |
                (m_mouseState.m_buttons[entry::MouseButton::Right] ? IMGUI_MBUT_RIGHT : 0) |
                (m_mouseState.m_buttons[entry::MouseButton::Middle] ? IMGUI_MBUT_MIDDLE : 0),
                m_mouseState.m_mz, uint16_t(m_width), uint16_t(m_height)
            );

            showExampleDialog(this);
            renderUI();

            // Update camera
            cameraUpdate(deltaTime * 0.01f, m_mouseState);

            // Update heightmap renderer
            m_heightmapRenderer.update(deltaTime, m_mouseState);

            imguiEndFrame();
            bgfx::frame(false);
            return true;
        }
        return false;
    }

private:
    void renderUI() {
        ImGui::SetNextWindowPos(ImVec2(m_width - m_width / 5.0f - 10.0f, 10.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(m_width / 5.0f, m_height / 3.0f), ImGuiCond_FirstUseEver);

        ImGui::Begin("Settings", nullptr, 0);
        
        // Performance stats
        ImGui::Text("Loading time: %.2f ms", m_heightmapRenderer.getLoadTime());
        if (m_heightmapRenderer.getCpuSmapTime() > 0.0f) {
            ImGui::Text("CPU SMap: %.2f ms", m_heightmapRenderer.getCpuSmapTime());
        }
        if (m_heightmapRenderer.getGpuSmapTime() > 0.0f) {
            ImGui::Text("GPU SMap: %.2f ms", m_heightmapRenderer.getGpuSmapTime());
        }
        
        // 添加调试信息显示
        ImGui::Separator();
        ImGui::Text("Debug Info:");
        ImGui::Text("App size: %dx%d", m_width, m_height);
        
        ImGui::End();
    }

    HeightmapRenderer m_heightmapRenderer;
    uint32_t m_width;
    uint32_t m_height;
    uint32_t m_debug;
    uint32_t m_reset;
    entry::MouseState m_mouseState;
    int64_t m_timeOffset;
};