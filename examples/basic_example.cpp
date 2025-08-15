#include <heightmaprenderer/heightmap_renderer.h>
#include <iostream>
#include <vector>
#include <iomanip>

int main() {
    std::cout << "HeightmapRenderer Library Example\n";
    std::cout << "Version: " << heightmap::HeightmapRenderer::getVersion() << "\n\n";

    // 创建渲染器实例
    heightmap::HeightmapRenderer renderer;

    // 创建简单的高度图数据（5x5）
    std::vector<std::vector<float>> heightData = {
        {0.0f, 0.5f, 1.0f, 0.5f, 0.0f},
        {0.5f, 1.0f, 1.5f, 1.0f, 0.5f},
        {1.0f, 1.5f, 2.0f, 1.5f, 1.0f},
        {0.5f, 1.0f, 1.5f, 1.0f, 0.5f},
        {0.0f, 0.5f, 1.0f, 0.5f, 0.0f}
    };

    // 加载高度图数据
    if (renderer.loadHeightmap(heightData)) {
        std::cout << "高度图加载成功!\n";
        std::cout << "尺寸: " << renderer.getWidth() << "x" << renderer.getHeight() << "\n\n";
    } else {
        std::cerr << "高度图加载失败!\n";
        return 1;
    }

    // 设置缩放
    renderer.setScale(1.0f, 5.0f, 1.0f);

    // 生成顶点数据
    auto vertices = renderer.generateVertices();
    std::cout << "生成了 " << vertices.size() << " 个顶点:\n";
    
    for (size_t i = 0; i < std::min(size_t(10), vertices.size()); ++i) {
        const auto& v = vertices[i];
        std::cout << "顶点 " << i << ": (" 
                  << std::fixed << std::setprecision(1)
                  << v.x << ", " << v.y << ", " << v.z << ")\n";
    }

    if (vertices.size() > 10) {
        std::cout << "... (显示前10个顶点)\n";
    }

    // 生成索引数据
    auto indices = renderer.generateIndices();
    std::cout << "\n生成了 " << indices.size() << " 个索引 ("
              << indices.size() / 3 << " 个三角形)\n";

    // 显示一些三角形
    std::cout << "前几个三角形的索引:\n";
    for (size_t i = 0; i < std::min(size_t(15), indices.size()); i += 3) {
        std::cout << "三角形 " << i/3 << ": [" 
                  << indices[i] << ", " << indices[i+1] << ", " << indices[i+2] << "]\n";
    }

    // 查询特定位置的高度
    std::cout << "\n高度查询:\n";
    for (int y = 0; y < renderer.getHeight(); y += 2) {
        for (int x = 0; x < renderer.getWidth(); x += 2) {
            float height = renderer.getHeightAt(x, y);
            std::cout << "位置 (" << x << ", " << y << ") 高度: " 
                      << std::fixed << std::setprecision(1) << height << "\n";
        }
    }

    std::cout << "\n示例执行完成!\n";
    return 0;
}