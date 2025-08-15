@echo off
echo 正在构建HeightmapRenderer...

if not exist .build mkdir .build
cd .build

cmake .. -DHEIGHTMAP_BUILD_SHARED_LIBS=ON
cmake --build . --config Release

if %ERRORLEVEL% neq 0 (
    echo 构建失败!
    pause
    exit /b 1
)

echo 构建成功!
echo.
echo 运行示例程序...
cd bin
basic_example.exe

pause