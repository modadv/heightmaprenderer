# lib/CMakeLists.txt
cmake_minimum_required(VERSION 3.14)

# 创建静态库
add_library(terrain-renderer STATIC
    src/renderer.cpp
    src/renderer_impl.cpp
)

# 设置包含目录
target_include_directories(terrain-renderer 
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 链接bgfx等依赖（从父项目继承）
target_link_libraries(terrain-renderer 
    PRIVATE
        bgfx
        bimg
        bx
)

# 添加父项目的包含路径（暂时需要）
target_include_directories(terrain-renderer PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src/common
    ${CMAKE_SOURCE_DIR}/external/bgfx.cmake/bgfx/include
    ${CMAKE_SOURCE_DIR}/external/bgfx.cmake/bx/include
    ${CMAKE_SOURCE_DIR}/external/bgfx.cmake/bimg/include
)

# C++14标准
target_compile_features(terrain-renderer PUBLIC cxx_std_14)

# 平台定义
if(WIN32)
    target_compile_definitions(terrain-renderer PRIVATE
        _WIN32
        _WIN64
        _CRT_SECURE_NO_WARNINGS
    )
endif()