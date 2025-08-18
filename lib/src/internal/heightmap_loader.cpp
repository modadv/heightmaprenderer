// lib/src/internal/heightmap_loader.cpp
#include "heightmap_loader.h"
#include <bimg/decode.h>
#include <bx/allocator.h>
#include <bx/file.h>
#include <cstdio>
#include <algorithm>

namespace terrain {
namespace internal {

HeightmapData::HeightmapData() {
}

HeightmapData::~HeightmapData() {
}

bool HeightmapData::LoadFromFile(const std::string& path) {
    // 使用bx读取文件
    bx::FileReader reader;
    bx::Error err;
    
    if (!bx::open(&reader, path.c_str(), &err)) {
        m_lastError = "Failed to open file: " + path;
        printf("[HeightmapData] %s\n", m_lastError.c_str());
        return false;
    }
    
    uint32_t size = (uint32_t)bx::getSize(&reader);
    if (size == 0) {
        m_lastError = "File is empty: " + path;
        bx::close(&reader);
        return false;
    }
    
    // 分配内存并读取
    std::vector<uint8_t> fileData(size);
    bx::read(&reader, fileData.data(), size, &err);
    bx::close(&reader);
    
    // 使用bimg解码图像
    bx::DefaultAllocator allocator;
    bimg::ImageContainer* image = bimg::imageParse(
        &allocator,
        fileData.data(),
        size,
        bimg::TextureFormat::R16  // 期望16位单通道
    );
    
    if (!image) {
        // 尝试其他格式
        image = bimg::imageParse(
            &allocator,
            fileData.data(),
            size,
            bimg::TextureFormat::R8  // 8位单通道
        );
        
        if (!image) {
            m_lastError = "Failed to parse image: " + path;
            return false;
        }
    }
    
    // 提取数据
    m_width = image->m_width;
    m_height = image->m_height;
    
    printf("[HeightmapData] Loaded %s: %dx%d, format=%d\n", 
           path.c_str(), m_width, m_height, image->m_format);
    
    // 转换为16位数据
    size_t pixelCount = m_width * m_height;
    m_data.resize(pixelCount);
    m_normalizedHeights.resize(pixelCount);
    
    if (image->m_format == bimg::TextureFormat::R16) {
        // 直接复制16位数据
        const uint16_t* src = (const uint16_t*)image->m_data;
        std::copy(src, src + pixelCount, m_data.begin());
    } else if (image->m_format == bimg::TextureFormat::R8) {
        // 从8位扩展到16位
        const uint8_t* src = (const uint8_t*)image->m_data;
        for (size_t i = 0; i < pixelCount; ++i) {
            m_data[i] = uint16_t(src[i]) * 257; // 扩展到16位范围
        }
    } else {
        // 其他格式，取第一个通道
        const uint8_t* src = (const uint8_t*)image->m_data;
        int bytesPerPixel = bimg::getBitsPerPixel(bimg::TextureFormat::Enum(image->m_format)) / 8;
        for (size_t i = 0; i < pixelCount; ++i) {
            m_data[i] = uint16_t(src[i * bytesPerPixel]) * 257;
        }
    }
    
    // 生成归一化高度
    for (size_t i = 0; i < pixelCount; ++i) {
        m_normalizedHeights[i] = float(m_data[i]) / 65535.0f;
    }
    
    // 清理
    bimg::imageFree(image);
    
    return true;
}

bool HeightmapData::LoadFromMemory(const uint8_t* data, size_t size) {
    if (!data || size == 0) {
        m_lastError = "Invalid memory buffer";
        return false;
    }
    
    // 使用bimg解码
    bx::DefaultAllocator allocator;
    bimg::ImageContainer* image = bimg::imageParse(
        &allocator,
        data,
        (uint32_t)size,
        bimg::TextureFormat::R16
    );
    
    if (!image) {
        m_lastError = "Failed to parse image from memory";
        return false;
    }
    
    // 与LoadFromFile相同的处理逻辑...
    m_width = image->m_width;
    m_height = image->m_height;
    
    size_t pixelCount = m_width * m_height;
    m_data.resize(pixelCount);
    m_normalizedHeights.resize(pixelCount);
    
    const uint16_t* src = (const uint16_t*)image->m_data;
    std::copy(src, src + pixelCount, m_data.begin());
    
    for (size_t i = 0; i < pixelCount; ++i) {
        m_normalizedHeights[i] = float(m_data[i]) / 65535.0f;
    }
    
    bimg::imageFree(image);
    return true;
}

std::vector<float> HeightmapData::GenerateSlopeMap() const {
    if (m_data.empty()) {
        return std::vector<float>();
    }
    
    std::vector<float> slopeMap(m_width * m_height * 2);
    
    for (int j = 0; j < m_height; ++j) {
        for (int i = 0; i < m_width; ++i) {
            // 获取相邻像素（带边界检查）
            int i1 = std::max(0, i - 1);
            int i2 = std::min(m_width - 1, i + 1);
            int j1 = std::max(0, j - 1);
            int j2 = std::min(m_height - 1, j + 1);
            
            // 获取高度值
            float z_l = m_normalizedHeights[i1 + m_width * j];
            float z_r = m_normalizedHeights[i2 + m_width * j];
            float z_b = m_normalizedHeights[i + m_width * j1];
            float z_t = m_normalizedHeights[i + m_width * j2];
            
            // 计算斜率
            float slope_x = float(m_width) * 0.5f * (z_r - z_l);
            float slope_y = float(m_height) * 0.5f * (z_t - z_b);
            
            // 存储到slope map
            slopeMap[2 * (i + m_width * j)] = slope_x;
            slopeMap[2 * (i + m_width * j) + 1] = slope_y;
        }
    }
    
    printf("[HeightmapData] Generated slope map for %dx%d heightmap\n", m_width, m_height);
    
    return slopeMap;
}

} // namespace internal
} // namespace terrain