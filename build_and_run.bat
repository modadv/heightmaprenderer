@echo off
echo Building HeightmapRenderer with BGFX support...

REM Check Git submodules
if not exist "external\bgfx.cmake\CMakeLists.txt" (
    echo Initializing Git submodules...
    git submodule update --init --recursive
    if %ERRORLEVEL% neq 0 (
        echo Git submodule initialization failed!
        pause
        exit /b 1
    )
)

if not exist .build mkdir .build
cd .build

echo Configuring CMake...
cmake .. -DHEIGHTMAP_BUILD_SHARED_LIBS=ON -DHEIGHTMAP_WITH_BGFX=ON

echo Starting build...
cmake --build . --config Release --parallel

if %ERRORLEVEL% neq 0 (
    echo Build failed!
    pause
    exit /b 1
)

echo Build successful!
echo.
echo Available example programs:
echo   1. basic_example.exe - Basic functionality example
echo   2. bgfx_example.exe - BGFX graphics rendering example
echo.

set /p choice="Choose example to run (1 or 2): "

cd bin

if "%choice%"=="1" (
    echo Running basic example...
    basic_example.exe
) else if "%choice%"=="2" (
    echo Running BGFX example...
    bgfx_example.exe
) else (
    echo Running basic example...
    basic_example.exe
)

pause