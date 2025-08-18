// lib/test/test_basic.cpp
#include <terrain/terrain.h>
#include <cstdio>

int main() {
    printf("Testing terrain-renderer library\n");
    
    terrain::Renderer renderer;
    terrain::Config config;
    config.windowWidth = 800;
    config.windowHeight = 600;
    
    if (!renderer.Initialize(&config)) {
        printf("Failed to initialize: %s\n", renderer.GetLastError());
        return -1;
    }
    
    renderer.LoadHeightmap("test.png");
    renderer.LoadTexture("texture.jpg");
    renderer.SetWireframe(true);
    renderer.SetScale(100.0f, 30.0f);
    
    renderer.Shutdown();
    
    printf("Test completed successfully\n");
    return 0;
}