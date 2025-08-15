#ifndef HEIGHTMAP_RENDERER_H
#define HEIGHTMAP_RENDERER_H

#include "export.h"
#include <vector>
#include <memory>
#include <string>

namespace heightmap {

struct Point3D {
    float x, y, z;
    Point3D(float x = 0.0f, float y = 0.0f, float z = 0.0f) : x(x), y(y), z(z) {}
};

class HEIGHTMAP_API HeightmapRenderer {
public:
    HeightmapRenderer();
    ~HeightmapRenderer();

    // 禁用拷贝构造和赋值
    HeightmapRenderer(const HeightmapRenderer&) = delete;
    HeightmapRenderer& operator=(const HeightmapRenderer&) = delete;

    // 支持移动语义
    HeightmapRenderer(HeightmapRenderer&&) noexcept;
    HeightmapRenderer& operator=(HeightmapRenderer&&) noexcept;

    // 加载高度图数据
    bool loadHeightmap(const std::vector<std::vector<float>>& heightData);
    bool loadHeightmapFromFile(const std::string& filename);

    // 渲染相关方法
    void setResolution(int width, int height);
    void setScale(float scaleX, float scaleY, float scaleZ);
    
    // 生成顶点数据
    std::vector<Point3D> generateVertices() const;
    std::vector<unsigned int> generateIndices() const;

    // 获取信息
    int getWidth() const;
    int getHeight() const;
    float getHeightAt(int x, int y) const;

    // 版本信息
    static std::string getVersion();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace heightmap

#endif // HEIGHTMAP_RENDERER_H