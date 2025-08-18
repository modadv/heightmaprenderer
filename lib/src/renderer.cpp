// lib/src/renderer.cpp
#include "terrain/terrain.h"
#include "renderer_impl.h"

namespace terrain {

Renderer::Renderer() 
    : m_impl(std::make_unique<RendererImpl>()) {
}

Renderer::~Renderer() = default;

bool Renderer::Initialize(const Config* config) {
    return m_impl->Initialize(config);
}

void Renderer::Shutdown() {
    m_impl->Shutdown();
}

void Renderer::SetRenderTarget(const RenderTarget& target) {
    m_impl->SetRenderTarget(target);
}

bool Renderer::LoadHeightmap(const std::string& path) {
    return m_impl->LoadHeightmap(path);
}

bool Renderer::LoadTexture(const std::string& path) {
    return m_impl->LoadTexture(path);
}

void Renderer::BeginFrame() {
    m_impl->BeginFrame();
}

void Renderer::SetViewProjection(const float* view, const float* proj) {
    m_impl->SetViewProjection(view, proj);
}

void Renderer::Draw() {
    m_impl->Draw();
}

void Renderer::EndFrame() {
    m_impl->EndFrame();
}

void Renderer::SetWireframe(bool enable) {
    m_impl->SetWireframe(enable);
}

void Renderer::SetScale(float horizontalScale, float verticalScale) {
    m_impl->SetScale(horizontalScale, verticalScale);
}

const char* Renderer::GetLastError() const {
    return m_impl->GetLastError();
}

} // namespace terrain