// lib/src/internal/heightmap_loader.h
#pragma once

#include <vector>
#include <string>
#include <memory>
#include <cstdint>

namespace terrain {
namespace internal {

// 高度图数据
class HeightmapData {
public:
    HeightmapData();
    ~HeightmapData();
    
    // 从文件加载
    bool LoadFromFile(const std::string& path);
    
    // 从内存加载
    bool LoadFromMemory(const uint8_t* data, size_t size);
    
    // 获取数据
    const uint16_t* GetRawData() const { return m_data.data(); }
    const std::vector<float>& GetNormalizedHeights() const { return m_normalizedHeights; }
    
    // 获取尺寸
    int GetWidth() const { return m_width; }
    int GetHeight() const { return m_height; }
    
    // 获取宽高比
    float GetAspectRatio() const { 
        return m_height > 0 ? float(m_width) / float(m_height) : 1.0f; 
    }
    
    // 获取错误信息
    const std::string& GetLastError() const { return m_lastError; }
    
    // 生成斜率图（SMap）
    std::vector<float> GenerateSlopeMap() const;
    
private:
    std::vector<uint16_t> m_data;           // 原始16位高度数据
    std::vector<float> m_normalizedHeights; // 归一化的高度 [0,1]
    int m_width = 0;
    int m_height = 0;
    std::string m_lastError;
};

} // namespace internal
} // namespace terrain