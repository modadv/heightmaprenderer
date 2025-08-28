# ========================================
# CMake 基础配置
# ========================================

# 指定 CMake 的最低版本要求
# 3.14 版本支持较新的特性，如更好的 MSVC 运行时库处理
cmake_minimum_required(VERSION 3.14)

# 定义项目名称和使用的编程语言
# project() 命令会自动创建一些变量，如 PROJECT_NAME, PROJECT_SOURCE_DIR 等
project(omniengine)

# ========================================
# 编译器和构建配置
# ========================================

# 设置 C++ 标准版本
# CMAKE_CXX_STANDARD 指定使用 C++14 标准
set(CMAKE_CXX_STANDARD 14)
# CMAKE_CXX_STANDARD_REQUIRED ON 表示必须支持指定的 C++ 标准，否则报错
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
# 这确保所有构建配置（Debug/Release）的可执行文件都输出到同一个 bin 目录
# CMAKE_RUNTIME_OUTPUT_DIRECTORY 是所有配置的默认输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
# CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG 是 Debug 配置的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
# CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE 是 Release 配置的输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

# ========================================
# Qt Support Option
# ========================================
option(USE_QT_ENTRY "Use Qt QML as entry point" ON)

# ========================================
# 外部依赖配置 - bgfx.cmake
# ========================================

# 配置 bgfx.cmake 的构建选项
# BGFX_BUILD_TOOLS ON 表示构建 bgfx 工具（如 shaderc 着色器编译器）
set(BGFX_BUILD_TOOLS ON CACHE BOOL "" FORCE)
# BGFX_BUILD_EXAMPLES OFF 表示不构建 bgfx 的示例程序
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# 添加 bgfx.cmake 作为子项目
# add_subdirectory 会处理指定目录中的 CMakeLists.txt
# 这会构建 bgfx、bimg、bx 库和 shaderc 工具
add_subdirectory(bgfx.cmake)

# ========================================
# 着色器编译配置
# ========================================

# 设置着色器相关的目录路径
# SHADER_SOURCE_DIR：着色器源代码（.sc 文件）所在目录
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders)
# SHADER_OUTPUT_DIR：编译后的着色器二进制文件输出目录
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)
# SHADER_INCLUDE_DIR：生成的着色器 C 头文件输出目录
set(SHADER_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include/generated/shaders)

# 创建输出目录（如果不存在）
# file(MAKE_DIRECTORY ...) 在配置时创建目录
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
file(MAKE_DIRECTORY ${SHADER_INCLUDE_DIR})

# varying.def.sc 文件定义了顶点着色器和片段着色器之间传递的变量
set(VARYING_DEF ${SHADER_SOURCE_DIR}/varying.def.sc)

# 设置着色器编译器的包含路径
# 这些路径用于查找着色器中 #include 的文件
set(SHADER_INCLUDE_PATHS
    # bgfx 内部着色器头文件（包含 bgfx_shader.sh 等核心文件）
    ${CMAKE_CURRENT_SOURCE_DIR}/bgfx.cmake/bgfx/src
    # 项目的着色器目录（包含自定义的 .sh 文件）
    ${SHADER_SOURCE_DIR}
    # common 目录（包含 common.sh 和 shaderlib.sh）
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
)

# 构建 shaderc 的包含路径参数列表
# 将每个路径转换为 -i "path" 格式
set(INCLUDE_ARGS "")
foreach(INCLUDE_PATH ${SHADER_INCLUDE_PATHS})
    list(APPEND INCLUDE_ARGS -i "${INCLUDE_PATH}")
endforeach()

# 定义所有需要编译的着色器
# 顶点着色器列表（vs_ 前缀）
set(VERTEX_SHADERS
    vs_terrain_render       # 地形渲染顶点着色器
)

# 片段着色器列表（fs_ 前缀）
set(FRAGMENT_SHADERS
    fs_terrain_render        # 地形渲染片段着色器（带纹理）
    fs_terrain_render_normal # 地形渲染片段着色器（法线显示模式）
)

# 计算着色器列表（cs_ 前缀）
set(COMPUTE_SHADERS
    cs_generate_smap          # 生成斜率贴图的计算着色器
    cs_terrain_init          # 地形初始化计算着色器
    cs_terrain_lod           # 地形 LOD（细节层次）计算着色器
    cs_terrain_update_draw   # 更新绘制参数的计算着色器
    cs_terrain_update_indirect # 更新间接绘制的计算着色器
)

# ========================================
# 着色器编译函数
# ========================================

# 定义一个函数来编译单个着色器
# 参数：
#   SHADER_NAME - 着色器文件名（不含扩展名）
#   SHADER_TYPE - 着色器类型（VERTEX/FRAGMENT/COMPUTE）
#   RENDERER - 目标渲染器（dx11/glsl/spirv/metal）
#   OUTPUT_DIR - 输出目录
function(compile_shader SHADER_NAME SHADER_TYPE RENDERER OUTPUT_DIR)
    # 根据着色器类型设置相应的标志和前缀
    if(${SHADER_TYPE} STREQUAL "VERTEX")
        set(TYPE_FLAG vertex)    # shaderc 的类型参数
        set(TYPE_PREFIX vs)       # 文件名前缀
    elseif(${SHADER_TYPE} STREQUAL "FRAGMENT")
        set(TYPE_FLAG fragment)
        set(TYPE_PREFIX fs)
    elseif(${SHADER_TYPE} STREQUAL "COMPUTE")
        set(TYPE_FLAG compute)
        set(TYPE_PREFIX cs)
    endif()
    
    # 根据渲染器和着色器类型设置正确的 profile
    # Profile 指定了着色器的目标版本/规范
    if(${RENDERER} STREQUAL "dx11")
        set(PLATFORM_NAME windows)
        if(${SHADER_TYPE} STREQUAL "VERTEX")
            set(PROFILE_VERSION vs_5_0)    # DirectX 11 顶点着色器 5.0
        elseif(${SHADER_TYPE} STREQUAL "FRAGMENT")
            set(PROFILE_VERSION ps_5_0)    # DirectX 11 像素着色器 5.0
        elseif(${SHADER_TYPE} STREQUAL "COMPUTE")
            set(PROFILE_VERSION cs_5_0)    # DirectX 11 计算着色器 5.0
        endif()
    elseif(${RENDERER} STREQUAL "glsl")
        # GLSL (OpenGL Shading Language)
        if(WIN32)
            set(PLATFORM_NAME windows)
        elseif(APPLE)
            set(PLATFORM_NAME osx)
        else()
            set(PLATFORM_NAME linux)
        endif()
        
        if(${SHADER_TYPE} STREQUAL "COMPUTE")
            set(PROFILE_VERSION 430)  # OpenGL 4.3 是支持计算着色器的最低版本
        else()
            if(APPLE)
                set(PROFILE_VERSION 410)  # macOS 只支持到 OpenGL 4.1
            else()
                set(PROFILE_VERSION 440)  # Windows/Linux 使用 OpenGL 4.4
            endif()
        endif()
    elseif(${RENDERER} STREQUAL "spirv")
        # SPIR-V (Vulkan 中间表示)
        if(WIN32)
            set(PLATFORM_NAME windows)
        elseif(APPLE)
            set(PLATFORM_NAME osx)
        else()
            set(PLATFORM_NAME linux)
        endif()
        set(PROFILE_VERSION spirv)
    elseif(${RENDERER} STREQUAL "metal")
        # Metal (Apple 的图形 API)
        set(PLATFORM_NAME osx)
        set(PROFILE_VERSION metal)
    endif()
    
    # 设置输入和输出文件路径
    set(INPUT_FILE ${SHADER_SOURCE_DIR}/${SHADER_NAME}.sc)
    set(OUTPUT_BIN ${OUTPUT_DIR}/${RENDERER}/${SHADER_NAME}.bin)
    set(OUTPUT_HEADER ${SHADER_INCLUDE_DIR}/${RENDERER}/${SHADER_NAME}.bin.h)
    
    # 创建渲染器特定的输出目录
    file(MAKE_DIRECTORY ${OUTPUT_DIR}/${RENDERER})
    file(MAKE_DIRECTORY ${SHADER_INCLUDE_DIR}/${RENDERER})
    
    # 构建 varyingdef 参数
    # 只有顶点和片段着色器需要 varying 定义
    if(NOT ${SHADER_TYPE} STREQUAL "COMPUTE")
        set(VARYINGDEF_ARG --varyingdef "${VARYING_DEF}")
    else()
        set(VARYINGDEF_ARG "")
    endif()
    
    # 添加自定义命令：生成着色器二进制文件
    # add_custom_command 创建一个构建规则
    add_custom_command(
        OUTPUT ${OUTPUT_BIN}                              # 输出文件
        COMMAND $<TARGET_FILE:shaderc>                    # 使用 shaderc 工具
            -f "${INPUT_FILE}"                            # 输入文件
            -o "${OUTPUT_BIN}"                            # 输出文件
            --type ${TYPE_FLAG}                           # 着色器类型
            --platform ${PLATFORM_NAME}                   # 目标平台
            --profile ${PROFILE_VERSION}                  # 着色器配置文件版本
            ${INCLUDE_ARGS}                               # 包含路径
            ${VARYINGDEF_ARG}                             # varying 定义
            $<$<CONFIG:Debug>:--debug>                    # Debug 配置时添加调试信息
            $<$<CONFIG:Release>:-O 3>                     # Release 配置时优化级别 3
        DEPENDS ${INPUT_FILE} ${VARYING_DEF} shaderc      # 依赖文件
        COMMENT "Compiling ${SHADER_TYPE} shader ${SHADER_NAME} for ${RENDERER} (${PROFILE_VERSION})"
        VERBATIM                                          # 不转义命令参数
    )
    
    # 添加自定义命令：生成 C 头文件（用于嵌入着色器）
    add_custom_command(
        OUTPUT ${OUTPUT_HEADER}
        COMMAND $<TARGET_FILE:shaderc>
            -f "${INPUT_FILE}"
            -o "${OUTPUT_HEADER}"
            --type ${TYPE_FLAG}
            --platform ${PLATFORM_NAME}
            --profile ${PROFILE_VERSION}
            --bin2c ${SHADER_NAME}_${RENDERER}            # 生成 C 数组的变量名
            ${INCLUDE_ARGS}
            ${VARYINGDEF_ARG}
            $<$<CONFIG:Debug>:--debug>
            $<$<CONFIG:Release>:-O 3>
        DEPENDS ${INPUT_FILE} ${VARYING_DEF} shaderc
        COMMENT "Generating header for ${SHADER_TYPE} shader ${SHADER_NAME} for ${RENDERER} (${PROFILE_VERSION})"
        VERBATIM
    )
    
    # 将生成的文件列表返回给父作用域
    set(${SHADER_NAME}_${RENDERER}_OUTPUTS 
        ${OUTPUT_BIN} 
        ${OUTPUT_HEADER} 
        PARENT_SCOPE    # PARENT_SCOPE 使变量在函数外部可见
    )
endfunction()

# ========================================
# 着色器编译执行
# ========================================

# 根据平台确定要编译的渲染器
if(WIN32)
    set(RENDERERS dx11 glsl spirv)    # Windows 支持 DirectX 11、OpenGL、Vulkan
elseif(APPLE)
    set(RENDERERS metal glsl spirv)   # macOS 支持 Metal、OpenGL、Vulkan
else()
    set(RENDERERS glsl spirv)         # Linux 支持 OpenGL、Vulkan
endif()

# 存储所有着色器输出文件，用于创建依赖关系
set(ALL_SHADER_OUTPUTS "")

# 编译所有顶点着色器
foreach(SHADER ${VERTEX_SHADERS})
    foreach(RENDERER ${RENDERERS})
        compile_shader(${SHADER} VERTEX ${RENDERER} ${SHADER_OUTPUT_DIR})
        list(APPEND ALL_SHADER_OUTPUTS ${${SHADER}_${RENDERER}_OUTPUTS})
    endforeach()
endforeach()

# 编译所有片段着色器
foreach(SHADER ${FRAGMENT_SHADERS})
    foreach(RENDERER ${RENDERERS})
        compile_shader(${SHADER} FRAGMENT ${RENDERER} ${SHADER_OUTPUT_DIR})
        list(APPEND ALL_SHADER_OUTPUTS ${${SHADER}_${RENDERER}_OUTPUTS})
    endforeach()
endforeach()

# 编译所有计算着色器
foreach(SHADER ${COMPUTE_SHADERS})
    foreach(RENDERER ${RENDERERS})
        # 检查渲染器是否支持计算着色器
        set(COMPUTE_SUPPORTED TRUE)
        
        # macOS 的 OpenGL 4.1 不支持计算着色器（需要 4.3+）
        if(${RENDERER} STREQUAL "glsl" AND APPLE)
            set(COMPUTE_SUPPORTED FALSE)
        endif()
        
        if(COMPUTE_SUPPORTED)
            compile_shader(${SHADER} COMPUTE ${RENDERER} ${SHADER_OUTPUT_DIR})
            list(APPEND ALL_SHADER_OUTPUTS ${${SHADER}_${RENDERER}_OUTPUTS})
        endif()
    endforeach()
endforeach()

# 创建一个自定义目标来编译所有着色器
# ALL 表示这个目标会在默认构建中执行
add_custom_target(shaders ALL DEPENDS ${ALL_SHADER_OUTPUTS})

# ========================================
# 主程序源文件收集
# ========================================
set(HEIGHTMAP_SOURCE_FILES
    src/heightmap/patch_tables.cpp
    src/heightmap/uniforms.cpp
    src/heightmap/heightmap_renderer.cpp
)

# 定义通用源文件列表（所有平台都需要的文件）
set(COMMON_SOURCE_FILES
    src/common/bgfx_utils.cpp       # bgfx utility functions
    src/common/camera.cpp            # camera control
    src/common/cube_atlas.cpp        # cube texture atlas
    src/common/example-glue.cpp     # example glue code
)

# 使用 GLOB 收集子目录中的所有源文件
# file(GLOB ...) 会查找匹配模式的所有文件
file(GLOB DEBUGDRAW_SOURCES src/common/debugdraw/*.cpp)    # debug drawing
file(GLOB ENTRY_SOURCES src/common/entry/*.cpp)            # entry point
file(GLOB FONT_SOURCES src/common/font/*.cpp)              # font rendering
file(GLOB IMGUI_SOURCES src/common/imgui/*.cpp)            # Dear ImGui UI
file(GLOB NANOVG_SOURCES src/common/nanovg/*.cpp)          # NanoVG vector graphics
file(GLOB PS_SOURCES src/common/ps/*.cpp)                  # particle system

# ========================================
# Qt Configuration
# ========================================

# Find Qt 5.15
find_package(Qt5 5.15 REQUIRED COMPONENTS Core Gui Qml Quick QuickControls2)

# Enable automatic MOC and RCC
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt specific compile definitions
add_compile_definitions(
    ENTRY_CONFIG_USE_NATIVE=0
)

# Add Qt resource files if they exist
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/res/qml/resources.qrc)
    set(QT_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/res/qml/resources.qrc)
endif()

message(STATUS "Qt entry enabled with Qt version: ${Qt5_VERSION}")

# ========================================
# Platform specific entry file filtering
# ========================================


# When using Qt, remove all native entry files
list(REMOVE_ITEM ENTRY_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_windows.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_x11.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_osx.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_ios.mm
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_android.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_asmjs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_sdl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_noop.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_html5.cpp
)

# ========================================
# Collect all source and header files
# ========================================

# Collect all source files and header files
file(GLOB_RECURSE ALL_SOURCE_FILES 
    src/*.cpp
    src/*.c
)

file(GLOB_RECURSE ALL_HEADER_FILES
    src/*.h
    src/*.hpp
)

# If there are .mm files (macOS)
if(APPLE)
    file(GLOB_RECURSE OBJC_SOURCES src/*.mm)
    list(APPEND ALL_SOURCE_FILES ${OBJC_SOURCES})
endif()

# Merge all files
set(ALL_PROJECT_FILES ${ALL_SOURCE_FILES} ${ALL_HEADER_FILES})

# Add Qt resources if available

list(APPEND ALL_PROJECT_FILES ${QT_RESOURCES})

# Remove all native entry files when using Qt
list(FILTER ALL_PROJECT_FILES EXCLUDE REGEX ".*entry_windows\\.cpp$")
list(FILTER ALL_PROJECT_FILES EXCLUDE REGEX ".*entry_x11\\.cpp$")
list(FILTER ALL_PROJECT_FILES EXCLUDE REGEX ".*entry_osx\\.mm$")
list(FILTER ALL_PROJECT_FILES EXCLUDE REGEX ".*entry_ios\\.mm$")
list(FILTER ALL_PROJECT_FILES EXCLUDE REGEX ".*entry_android\\.cpp$")
list(FILTER ALL_PROJECT_FILES EXCLUDE REGEX ".*entry_asmjs\\.cpp$")
list(FILTER ALL_PROJECT_FILES EXCLUDE REGEX ".*entry_html5\\.cpp$")
list(FILTER ALL_PROJECT_FILES EXCLUDE REGEX ".*entry_glfw\\.cpp$")
list(FILTER ALL_PROJECT_FILES EXCLUDE REGEX ".*entry_sdl\\.cpp$")
list(FILTER ALL_PROJECT_FILES EXCLUDE REGEX ".*entry_noop\\.cpp$")

# ========================================
# Create executable target
# ========================================
add_executable(${PROJECT_NAME} ${ALL_PROJECT_FILES})

# ========================================
# Set include directories
# ========================================
target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src                    # Project source directory
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common             # Common directory
    # bgfx related include directories
    ${CMAKE_CURRENT_SOURCE_DIR}/bgfx.cmake/bx/include        # bx utility library headers
    ${CMAKE_CURRENT_SOURCE_DIR}/bgfx.cmake/bimg/include      # bimg image library headers    # STB single header libraries
    ${SHADER_INCLUDE_DIR}  # Generated shader header directory
)

# Platform specific include directories
if(WIN32)
    # Windows platform needs MSVC compatibility headers
    target_include_directories(${PROJECT_NAME} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/bgfx.cmake/bx/include/compat/msvc
    )
elseif(APPLE)
    # macOS platform needs OSX compatibility headers
    target_include_directories(${PROJECT_NAME} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/bgfx.cmake/bx/include/compat/osx
    )
endif()

# ========================================
# Set preprocessor definitions
# ========================================

# target_compile_definitions sets compile-time macros
target_compile_definitions(${PROJECT_NAME} PUBLIC
    # Entry system needs this macro to properly implement main function
    ENTRY_CONFIG_IMPLEMENT_MAIN=1
    
    # Set debug macro based on build configuration
    # $<$<CONFIG:Debug>:...> is a generator expression that only applies in Debug configuration
    $<$<CONFIG:Debug>:BX_CONFIG_DEBUG=1>
    $<$<CONFIG:Release>:BX_CONFIG_DEBUG=0>
)

# Platform specific preprocessor definitions
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        BX_PLATFORM_WINDOWS=1
        _WIN32                          # Windows 32-bit identifier (also defined on 64-bit)
        _WIN64                          # Windows 64-bit identifier
        _CRT_SECURE_NO_WARNINGS         # Disable MSVC security warnings
        __STDC_FORMAT_MACROS            # Enable C99 format macros
    )
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        BX_PLATFORM_OSX=1
        __APPLE__                       # Apple platform identifier
    )
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        BX_PLATFORM_LINUX=1
        __linux__                       # Linux platform identifier
    )
endif()

# ========================================
# Link libraries
# ========================================

# Conditionally link Qt libraries
target_link_libraries(${PROJECT_NAME} PUBLIC
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
    Qt5::QuickControls2
    bgfx
    bx
    bimg
)
# If bgfx.cmake built third party library targets, link them
if(TARGET dear-imgui)
    target_link_libraries(${PROJECT_NAME} PUBLIC dear-imgui)
endif()
if(TARGET meshoptimizer)
    target_link_libraries(${PROJECT_NAME} PUBLIC meshoptimizer)
endif()

# Ensure shaders are compiled before main program
# add_dependencies creates dependencies between targets
add_dependencies(${PROJECT_NAME} shaders)

# ========================================
# Platform specific link configuration
# ========================================

if(WIN32)
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/SUBSYSTEM:CONSOLE"
    )
    # Windows platform system libraries
    target_link_libraries(${PROJECT_NAME} PUBLIC 
        psapi    # Process Status API for process information
    )
    
    # Qt applications should use Windows subsystem
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        WIN32_EXECUTABLE TRUE
        LINK_FLAGS_DEBUG "/SUBSYSTEM:WINDOWS"
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS"
    )
endif()

# ========================================
# Resource file copy configuration
# ========================================

# Use add_custom_command to copy resource files after build
# POST_BUILD means execute after target is built

# Copy all runtime directory contents to bin directory
# This includes fonts, images, meshes, shaders, textures and other runtime resources
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/res/runtime $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying runtime contents to bin directory..."
)

# Copy compiled shaders, overwriting default shader files
foreach(RENDERER ${RENDERERS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SHADER_OUTPUT_DIR}/${RENDERER} $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/${RENDERER}
        COMMENT "Copying compiled ${RENDERER} shaders..."
    )
endforeach()

# ========================================
# Visual Studio specific configuration
# ========================================

if(MSVC)
    # Set VS debugger working directory
    # This ensures when pressing F5 to debug in VS, the program can find resource files
    set_property(TARGET ${PROJECT_NAME} PROPERTY 
        VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# Set project as VS startup project
# This makes omniengine the default startup project when opening the solution
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY 
    VS_STARTUP_PROJECT ${PROJECT_NAME}
)

# ========================================
# Source file organization (folder structure in IDE)
# ========================================

# Use TREE option to automatically create virtual folders based on file system structure
# PREFIX sets the root name of virtual folders
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Source Files" FILES ${ALL_PROJECT_FILES})

# Organize shader files separately
file(GLOB_RECURSE SHADER_FILES
    src/shaders/*.sc
    src/shaders/*.sh
    src/common/*.sh
)
source_group(TREE ${CMAKE_CURRENT_SOURCE_DIR}/src PREFIX "Shaders" FILES ${SHADER_FILES})

# ========================================
# Installation configuration (for packaging)
# ========================================

# Set installation directory prefix
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Installation Directory")

# Install executable to root directory
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION .
)

# Install all runtime directory contents
install(DIRECTORY ${CMAKE_SOURCE_DIR}/res/runtime/
    DESTINATION .
)

# Install compiled shaders (overwrite defaults)
foreach(RENDERER ${RENDERERS})
    if(EXISTS ${SHADER_OUTPUT_DIR}/${RENDERER})
        install(DIRECTORY ${SHADER_OUTPUT_DIR}/${RENDERER}/
            DESTINATION shaders/${RENDERER}
        )
    endif()
endforeach()

# ========================================
# Configuration information output
# ========================================

# Output configuration information to help debug and confirm settings
message(STATUS "========================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Renderers: ${RENDERERS}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Shader output: ${SHADER_OUTPUT_DIR}")
message(STATUS "Shader headers: ${SHADER_INCLUDE_DIR}")

message(STATUS "Entry system: Qt QML")
message(STATUS "Qt version: ${Qt5_VERSION}")
message(STATUS "========================================")