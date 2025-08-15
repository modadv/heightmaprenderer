#!/bin/bash
echo "正在构建HeightmapRenderer..."

mkdir -p .build
cd .build

cmake .. -DHEIGHTMAP_BUILD_SHARED_LIBS=ON
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "构建失败!"
    exit 1
fi

echo "构建成功!"
echo ""
echo "运行示例程序..."
cd bin
./basic_example