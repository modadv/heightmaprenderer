// lib/include/terrain/terrain.h
#pragma once

#include <memory>
#include <string>

namespace terrain {

// 前向声明
class RendererImpl;

// 渲染目标描述
struct RenderTarget {
    void* nativeHandle;      // HWND, QWidget*, etc.
    uint32_t width;
    uint32_t height;
    enum Type {
        Window,              // 窗口句柄
        Texture,            // 渲染到纹理
        Framebuffer         // 自定义帧缓冲
    } type = Window;
};

// 简单配置
struct Config {
    int windowWidth = 1280;
    int windowHeight = 720;
    bool debug = false;
};

// 主渲染器类
class Renderer {
public:
    Renderer();
    ~Renderer();
    
    // 基础生命周期
    bool Initialize(const Config* config = nullptr);
    void Shutdown();
    
    // 设置渲染目标
    void SetRenderTarget(const RenderTarget& target);
    
    // 资源加载
    bool LoadHeightmap(const std::string& path);
    bool LoadTexture(const std::string& path);
    
    // 渲染
    void BeginFrame();
    void SetViewProjection(const float* view, const float* proj);
    void Draw();
    void EndFrame();
    
    // 基础设置
    void SetWireframe(bool enable);
    void SetScale(float horizontalScale, float verticalScale);
    
    // 错误处理
    const char* GetLastError() const;
    
private:
    std::unique_ptr<RendererImpl> m_impl;
};

// 简单相机辅助类
class SimpleCamera {
public:
    SimpleCamera();
    ~SimpleCamera();
    
    void SetPosition(float x, float y, float z);
    void LookAt(float eyeX, float eyeY, float eyeZ,
                float targetX, float targetY, float targetZ,
                float upX = 0, float upY = 1, float upZ = 0);
    
    void MoveForward(float distance);
    void MoveRight(float distance);
    void MoveUp(float distance);
    void Rotate(float yaw, float pitch);
    
    const float* GetViewMatrix() const;
    const float* GetProjectionMatrix() const;
    
    void SetFov(float fov);
    void SetAspectRatio(float aspect);
    
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace terrain