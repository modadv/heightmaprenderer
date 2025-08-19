#include "uniforms.h"

namespace heightmap {

void Uniforms::init() {
    m_paramsHandle = bgfx::createUniform("u_params", bgfx::UniformType::Vec4, tables::kNumVec4);
    m_aspectParamsHandle = bgfx::createUniform("u_aspectParams", bgfx::UniformType::Vec4);

    cull = 1.0f;
    freeze = 0.0f;
    gpuSubd = 3.0f;
    terrainHalfWidth = 1.0f;
    terrainHalfHeight = 1.0f;
}

void Uniforms::submit() {
    bgfx::setUniform(m_paramsHandle, params, tables::kNumVec4);
    
    float aspectParams[4] = { terrainHalfWidth, terrainHalfHeight, 0.0f, 0.0f };
    bgfx::setUniform(m_aspectParamsHandle, aspectParams);
}

void Uniforms::destroy() {
    bgfx::destroy(m_paramsHandle);
    bgfx::destroy(m_aspectParamsHandle);
}

} // namespace heightmap