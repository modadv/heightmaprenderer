cmake_minimum_required(VERSION 3.10)
project(heightmaprenderer VERSION 1.0.0 LANGUAGES CXX)

# 设置C++14标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 包含模块
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# 选项
option(HEIGHTMAP_BUILD_EXAMPLES "Build examples" ON)
option(HEIGHTMAP_BUILD_SHARED_LIBS "Build shared libraries" ON)

# 设置库类型
if(HEIGHTMAP_BUILD_SHARED_LIBS)
    set(LIBRARY_TYPE SHARED)
else()
    set(LIBRARY_TYPE STATIC)
endif()

# 创建库目标
add_library(heightmaprenderer ${LIBRARY_TYPE}
    src/heightmap_renderer.cpp
)

# 设置目标属性
set_target_properties(heightmaprenderer PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

# 包含目录
target_include_directories(heightmaprenderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# 编译定义
if(HEIGHTMAP_BUILD_SHARED_LIBS)
    target_compile_definitions(heightmaprenderer
        PUBLIC HEIGHTMAP_SHARED_LIBS
        PRIVATE HEIGHTMAP_BUILDING_DLL
    )
endif()

# 编译器特定设置
if(MSVC)
    target_compile_options(heightmaprenderer PRIVATE /W4)
    if(HEIGHTMAP_BUILD_SHARED_LIBS)
        target_compile_definitions(heightmaprenderer PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
else()
    target_compile_options(heightmaprenderer PRIVATE -Wall -Wextra -Wpedantic)
endif()

# 创建别名目标
add_library(heightmaprenderer::heightmaprenderer ALIAS heightmaprenderer)

# 安装目标
install(TARGETS heightmaprenderer
    EXPORT heightmaprendererTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# 安装头文件
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# 导出目标
install(EXPORT heightmaprendererTargets
    FILE heightmaprendererTargets.cmake
    NAMESPACE heightmaprenderer::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/heightmaprenderer
)

# 配置包配置文件
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/heightmaprendererConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/heightmaprendererConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/heightmaprenderer
)

# 版本配置文件
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/heightmaprendererConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# 安装配置文件
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/heightmaprendererConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/heightmaprendererConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/heightmaprenderer
)

# 构建示例
if(HEIGHTMAP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()