#pragma once

#include <bgfx/bgfx.h>
#include "patch_tables.h"
namespace heightmap {

class Uniforms {
public:
    void init();
    void submit();
    void destroy();

    // Uniform parameters
    union {
        struct {
            float dmapFactor;
            float lodFactor; 
            float cull;
            float freeze;

            float gpuSubd;
            float padding0;
            float padding1;
            float padding2;

            float terrainHalfWidth;
            float terrainHalfHeight;
        };
        float params[tables::kNumVec4 * 4];
    };

private:
    bgfx::UniformHandle m_paramsHandle;
    bgfx::UniformHandle m_aspectParamsHandle;
};

} // namespace heightmap