#include <heightmaprenderer/heightmap_renderer.h>
#include <iostream>
#include <vector>
#include <iomanip>
#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

void printSystemInfo() {
#ifdef _WIN32
    char buffer[MAX_PATH];
    GetCurrentDirectoryA(MAX_PATH, buffer);
    std::cout << "Current working directory: " << buffer << "\n";
    
    HMODULE hModule = GetModuleHandleA("heightmaprenderer.dll");
    if (hModule) {
        GetModuleFileNameA(hModule, buffer, MAX_PATH);
        std::cout << "Found DLL: " << buffer << "\n";
    } else {
        std::cout << "Warning: heightmaprenderer.dll not found\n";
    }
#endif
    std::cout << "\n";
}

int main() {
    std::cout << "=== HeightmapRenderer Library Example ===\n";
    std::cout << "Version: " << heightmap::HeightmapRenderer::getVersion() << "\n";
    
    printSystemInfo();

    try {
        // Create renderer instance
        heightmap::HeightmapRenderer renderer;

        // Create simple heightmap data (5x5)
        std::vector<std::vector<float> > heightData(5);
        heightData[0] = std::vector<float>(5);
        heightData[0][0] = 0.0f; heightData[0][1] = 0.5f; heightData[0][2] = 1.0f; heightData[0][3] = 0.5f; heightData[0][4] = 0.0f;
        heightData[1] = std::vector<float>(5);
        heightData[1][0] = 0.5f; heightData[1][1] = 1.0f; heightData[1][2] = 1.5f; heightData[1][3] = 1.0f; heightData[1][4] = 0.5f;
        heightData[2] = std::vector<float>(5);
        heightData[2][0] = 1.0f; heightData[2][1] = 1.5f; heightData[2][2] = 2.0f; heightData[2][3] = 1.5f; heightData[2][4] = 1.0f;
        heightData[3] = std::vector<float>(5);
        heightData[3][0] = 0.5f; heightData[3][1] = 1.0f; heightData[3][2] = 1.5f; heightData[3][3] = 1.0f; heightData[3][4] = 0.5f;
        heightData[4] = std::vector<float>(5);
        heightData[4][0] = 0.0f; heightData[4][1] = 0.5f; heightData[4][2] = 1.0f; heightData[4][3] = 0.5f; heightData[4][4] = 0.0f;

        // Load heightmap data
        if (renderer.loadHeightmap(heightData)) {
            std::cout << "✓ Heightmap loaded successfully!\n";
            std::cout << "Size: " << renderer.getWidth() << "x" << renderer.getHeight() << "\n\n";
        } else {
            std::cerr << "✗ Failed to load heightmap!\n";
            return 1;
        }

        // Set scale
        renderer.setScale(1.0f, 5.0f, 1.0f);

        // Generate vertex data - Use Point3D instead of Vertex
        std::vector<heightmap::Point3D> vertices = renderer.generateVertices();
        std::cout << "Generated " << vertices.size() << " vertices:\n";
        
        size_t maxVertices = (std::min)(static_cast<size_t>(10), vertices.size());
        for (size_t i = 0; i < maxVertices; ++i) {
            const heightmap::Point3D& v = vertices[i];
            std::cout << "  Vertex " << std::setw(2) << i << ": (" 
                      << std::fixed << std::setprecision(1)
                      << std::setw(4) << v.x << ", " 
                      << std::setw(4) << v.y << ", " 
                      << std::setw(4) << v.z << ")\n";
        }

        if (vertices.size() > 10) {
            std::cout << "  ... (showing first 10 vertices)\n";
        }

        // Generate index data
        std::vector<unsigned int> indices = renderer.generateIndices();
        std::cout << "\nGenerated " << indices.size() << " indices ("
                  << indices.size() / 3 << " triangles)\n";

        // Show some triangles
        std::cout << "First few triangle indices:\n";
        size_t maxIndices = (std::min)(static_cast<size_t>(15), indices.size());
        for (size_t i = 0; i < maxIndices; i += 3) {
            if (i + 2 < indices.size()) {
                std::cout << "  Triangle " << std::setw(2) << i/3 << ": [" 
                          << std::setw(2) << indices[i] << ", " 
                          << std::setw(2) << indices[i+1] << ", " 
                          << std::setw(2) << indices[i+2] << "]\n";
            }
        }

        // Query height at specific positions
        std::cout << "\nHeight queries:\n";
        for (int y = 0; y < renderer.getHeight(); y += 2) {
            for (int x = 0; x < renderer.getWidth(); x += 2) {
                float height = renderer.getHeightAt(x, y);
                std::cout << "  Position (" << x << ", " << y << ") height: " 
                          << std::fixed << std::setprecision(1) << height << "\n";
            }
        }

        std::cout << "\n✓ Example completed successfully!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "✗ Exception: " << e.what() << "\n";
        return 1;
    } catch (...) {
        std::cerr << "✗ Unknown exception occurred\n";
        return 1;
    }

#ifdef _WIN32
    std::cout << "\nPress any key to exit...";
    system("pause >nul");
#endif

    return 0;
}