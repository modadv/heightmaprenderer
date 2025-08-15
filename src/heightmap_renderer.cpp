#include "heightmaprenderer/heightmap_renderer.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace heightmap {

class HeightmapRenderer::Impl {
public:
    std::vector<std::vector<float>> heightData;
    int width = 0;
    int height = 0;
    float scaleX = 1.0f;
    float scaleY = 1.0f;
    float scaleZ = 1.0f;

    bool loadFromVector(const std::vector<std::vector<float>>& data) {
        if (data.empty() || data[0].empty()) {
            return false;
        }

        height = static_cast<int>(data.size());
        width = static_cast<int>(data[0].size());
        
        // 验证所有行都有相同的列数
        for (const auto& row : data) {
            if (static_cast<int>(row.size()) != width) {
                return false;
            }
        }

        heightData = data;
        return true;
    }

    std::vector<Point3D> generateVertices() const {
        std::vector<Point3D> vertices;
        vertices.reserve(width * height);

        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                float worldX = x * scaleX;
                float worldY = heightData[y][x] * scaleY;
                float worldZ = y * scaleZ;
                vertices.emplace_back(worldX, worldY, worldZ);
            }
        }

        return vertices;
    }

    std::vector<unsigned int> generateIndices() const {
        std::vector<unsigned int> indices;
        
        if (width < 2 || height < 2) {
            return indices;
        }

        indices.reserve((width - 1) * (height - 1) * 6);

        for (int y = 0; y < height - 1; ++y) {
            for (int x = 0; x < width - 1; ++x) {
                unsigned int topLeft = y * width + x;
                unsigned int topRight = y * width + (x + 1);
                unsigned int bottomLeft = (y + 1) * width + x;
                unsigned int bottomRight = (y + 1) * width + (x + 1);

                // 第一个三角形
                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                // 第二个三角形
                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }

        return indices;
    }
};

HeightmapRenderer::HeightmapRenderer() : pImpl(std::make_unique<Impl>()) {}

HeightmapRenderer::~HeightmapRenderer() = default;

HeightmapRenderer::HeightmapRenderer(HeightmapRenderer&&) noexcept = default;

HeightmapRenderer& HeightmapRenderer::operator=(HeightmapRenderer&&) noexcept = default;

bool HeightmapRenderer::loadHeightmap(const std::vector<std::vector<float>>& heightData) {
    return pImpl->loadFromVector(heightData);
}

bool HeightmapRenderer::loadHeightmapFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    std::vector<std::vector<float>> data;
    std::string line;
    
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::vector<float> row;
        float value;
        
        while (iss >> value) {
            row.push_back(value);
        }
        
        if (!row.empty()) {
            data.push_back(std::move(row));
        }
    }

    return loadHeightmap(data);
}

void HeightmapRenderer::setResolution(int width, int height) {
    if (width > 0 && height > 0) {
        pImpl->width = width;
        pImpl->height = height;
    }
}

void HeightmapRenderer::setScale(float scaleX, float scaleY, float scaleZ) {
    pImpl->scaleX = scaleX;
    pImpl->scaleY = scaleY;
    pImpl->scaleZ = scaleZ;
}

std::vector<Point3D> HeightmapRenderer::generateVertices() const {
    return pImpl->generateVertices();
}

std::vector<unsigned int> HeightmapRenderer::generateIndices() const {
    return pImpl->generateIndices();
}

int HeightmapRenderer::getWidth() const {
    return pImpl->width;
}

int HeightmapRenderer::getHeight() const {
    return pImpl->height;
}

float HeightmapRenderer::getHeightAt(int x, int y) const {
    if (x >= 0 && x < pImpl->width && y >= 0 && y < pImpl->height) {
        return pImpl->heightData[y][x];
    }
    return 0.0f;
}

std::string HeightmapRenderer::getVersion() {
    return "1.0.0";
}

} // namespace heightmap