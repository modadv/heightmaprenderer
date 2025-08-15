#!/bin/bash
echo "Building HeightmapRenderer with BGFX support..."

# Check Git submodules
if [ ! -f "external/bgfx.cmake/CMakeLists.txt" ]; then
    echo "Initializing Git submodules..."
    git submodule update --init --recursive
    if [ $? -ne 0 ]; then
        echo "Git submodule initialization failed!"
        exit 1
    fi
fi

mkdir -p .build
cd .build

echo "Configuring CMake..."
cmake .. -DHEIGHTMAP_BUILD_SHARED_LIBS=ON -DHEIGHTMAP_WITH_BGFX=ON

echo "Starting build..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "Build failed!"
    exit 1
fi

echo "Build successful!"
echo ""
echo "Available example programs:"
echo "  1. basic_example - Basic functionality example"
echo "  2. bgfx_example - BGFX graphics rendering example"
echo ""

read -p "Choose example to run (1 or 2): " choice

cd bin

if [ "$choice" == "1" ]; then
    echo "Running basic example..."
    ./basic_example
elif [ "$choice" == "2" ]; then
    echo "Running BGFX example..."
    ./bgfx_example
else
    echo "Running basic example..."
    ./basic_example
fi