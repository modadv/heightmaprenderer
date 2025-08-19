#pragma once
#include "heightmap/heightmap_renderer.h"
#include "common/common.h"
#include "common/camera.h"
#include "common/imgui/imgui.h"
#include "common/bgfx_utils.h"

namespace heightmap {
        
    class ExampleHeightmap : public entry::AppI {
    public:
        ExampleHeightmap(const char* name, const char* description, const char* url)
            : entry::AppI(name, description, url) {}

        void init(int32_t argc, const char* const* argv, uint32_t width, uint32_t height) override {
            Args args(argc, argv);
            
            m_width = width;
            m_height = height;
            m_debug = BGFX_DEBUG_NONE;
            m_reset = BGFX_RESET_NONE;

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
        }

        int shutdown() override {
            m_heightmapRenderer.shutdown();
            cameraDestroy();
            imguiDestroy();
            bgfx::shutdown();
            return 0;
        }

        bool update() override {
            if (!entry::processEvents(m_width, m_height, m_debug, m_reset, &m_mouseState)) {
                int64_t now = bx::getHPCounter();
                static int64_t last = now;
                const int64_t frameTime = now - last;
                last = now;
                const double freq = double(bx::getHPFrequency());
                const float deltaTime = float(frameTime / freq);

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

            // Controls will be moved to HeightmapRenderer's UI method
            // For now, just show basic info
            
            ImGui::End();
        }

        heightmap::HeightmapRenderer m_heightmapRenderer;
        uint32_t m_width;
        uint32_t m_height;
        uint32_t m_debug;
        uint32_t m_reset;
        entry::MouseState m_mouseState;
        int64_t m_timeOffset;
    };
} // namespace heightmap