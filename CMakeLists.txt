cmake_minimum_required(VERSION 3.14)
project(heightmaprenderer)

# 设置C++标准
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/bin)

# 确保构建bgfx工具
set(BGFX_BUILD_TOOLS ON CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# 添加bgfx.cmake子项目
add_subdirectory(external/bgfx.cmake)

# ========================================
# 着色器编译配置
# ========================================

# 设置着色器目录
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders)
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)
set(SHADER_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include/generated/shaders)

# 创建输出目录
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
file(MAKE_DIRECTORY ${SHADER_INCLUDE_DIR})

# varying.def.sc文件路径
set(VARYING_DEF ${SHADER_SOURCE_DIR}/varying.def.sc)

# 着色器包含路径 - 确保顺序正确，bgfx的路径在前
set(SHADER_INCLUDE_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/src      # bgfx 内部着色器头文件（优先）
    ${SHADER_SOURCE_DIR}                                          # 项目的着色器目录
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common                        # common.sh 和 shaderlib.sh
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/examples/common  # bgfx examples common
)

# 构建包含路径参数
set(INCLUDE_ARGS "")
foreach(INCLUDE_PATH ${SHADER_INCLUDE_PATHS})
    list(APPEND INCLUDE_ARGS -i "${INCLUDE_PATH}")
endforeach()

# 定义着色器列表
set(VERTEX_SHADERS
    vs_terrain_render
)

set(FRAGMENT_SHADERS
    fs_terrain_render
    fs_terrain_render_normal
)

set(COMPUTE_SHADERS
    cs_generate_smap
    cs_terrain_init
    cs_terrain_lod
    cs_terrain_update_draw
    cs_terrain_update_indirect
)

# 函数：编译单个着色器
function(compile_shader SHADER_NAME SHADER_TYPE RENDERER OUTPUT_DIR)
    # 确定着色器类型标记
    if(${SHADER_TYPE} STREQUAL "VERTEX")
        set(TYPE_FLAG vertex)
        set(TYPE_PREFIX vs)
    elseif(${SHADER_TYPE} STREQUAL "FRAGMENT")
        set(TYPE_FLAG fragment)
        set(TYPE_PREFIX fs)
    elseif(${SHADER_TYPE} STREQUAL "COMPUTE")
        set(TYPE_FLAG compute)
        set(TYPE_PREFIX cs)
    endif()
    
    # 根据渲染器和着色器类型设置正确的profile
    if(${RENDERER} STREQUAL "dx11")
        set(PLATFORM_NAME windows)
        if(${SHADER_TYPE} STREQUAL "VERTEX")
            set(PROFILE_VERSION vs_5_0)
        elseif(${SHADER_TYPE} STREQUAL "FRAGMENT")
            set(PROFILE_VERSION ps_5_0)
        elseif(${SHADER_TYPE} STREQUAL "COMPUTE")
            set(PROFILE_VERSION cs_5_0)
        endif()
    elseif(${RENDERER} STREQUAL "glsl")
        if(WIN32)
            set(PLATFORM_NAME windows)
        elseif(APPLE)
            set(PLATFORM_NAME osx)
        else()
            set(PLATFORM_NAME linux)
        endif()
        
        if(${SHADER_TYPE} STREQUAL "COMPUTE")
            set(PROFILE_VERSION 430)  # OpenGL 4.3 minimum for compute shaders
        else()
            if(APPLE)
                set(PROFILE_VERSION 410)  # macOS OpenGL 4.1
            else()
                set(PROFILE_VERSION 440)  # Windows/Linux OpenGL 4.4
            endif()
        endif()
    elseif(${RENDERER} STREQUAL "spirv")
        if(WIN32)
            set(PLATFORM_NAME windows)
        elseif(APPLE)
            set(PLATFORM_NAME osx)
        else()
            set(PLATFORM_NAME linux)
        endif()
        set(PROFILE_VERSION spirv)
    elseif(${RENDERER} STREQUAL "metal")
        set(PLATFORM_NAME osx)
        set(PROFILE_VERSION metal)
    endif()
    
    # 输入和输出文件路径
    set(INPUT_FILE ${SHADER_SOURCE_DIR}/${SHADER_NAME}.sc)
    set(OUTPUT_BIN ${OUTPUT_DIR}/${RENDERER}/${SHADER_NAME}.bin)
    set(OUTPUT_HEADER ${SHADER_INCLUDE_DIR}/${RENDERER}/${SHADER_NAME}.bin.h)
    
    # 创建渲染器特定的输出目录
    file(MAKE_DIRECTORY ${OUTPUT_DIR}/${RENDERER})
    file(MAKE_DIRECTORY ${SHADER_INCLUDE_DIR}/${RENDERER})
    
    # 构建varyingdef参数（仅对顶点和片段着色器）
    if(NOT ${SHADER_TYPE} STREQUAL "COMPUTE")
        set(VARYINGDEF_ARG --varyingdef "${VARYING_DEF}")
    else()
        set(VARYINGDEF_ARG "")
    endif()
    
    # 生成二进制文件的命令
    add_custom_command(
        OUTPUT ${OUTPUT_BIN}
        COMMAND $<TARGET_FILE:shaderc>
            -f "${INPUT_FILE}"
            -o "${OUTPUT_BIN}"
            --type ${TYPE_FLAG}
            --platform ${PLATFORM_NAME}
            --profile ${PROFILE_VERSION}
            ${INCLUDE_ARGS}
            ${VARYINGDEF_ARG}
            $<$<CONFIG:Debug>:--debug>
            $<$<CONFIG:Release>:-O 3>
        DEPENDS ${INPUT_FILE} ${VARYING_DEF} shaderc
        COMMENT "Compiling ${SHADER_TYPE} shader ${SHADER_NAME} for ${RENDERER} (${PROFILE_VERSION})"
        VERBATIM
    )
    
    # 生成C头文件的命令
    add_custom_command(
        OUTPUT ${OUTPUT_HEADER}
        COMMAND $<TARGET_FILE:shaderc>
            -f "${INPUT_FILE}"
            -o "${OUTPUT_HEADER}"
            --type ${TYPE_FLAG}
            --platform ${PLATFORM_NAME}
            --profile ${PROFILE_VERSION}
            --bin2c ${SHADER_NAME}_${RENDERER}
            ${INCLUDE_ARGS}
            ${VARYINGDEF_ARG}
            $<$<CONFIG:Debug>:--debug>
            $<$<CONFIG:Release>:-O 3>
        DEPENDS ${INPUT_FILE} ${VARYING_DEF} shaderc
        COMMENT "Generating header for ${SHADER_TYPE} shader ${SHADER_NAME} for ${RENDERER} (${PROFILE_VERSION})"
        VERBATIM
    )
    
    # 返回生成的文件列表
    set(${SHADER_NAME}_${RENDERER}_OUTPUTS 
        ${OUTPUT_BIN} 
        ${OUTPUT_HEADER} 
        PARENT_SCOPE
    )
endfunction()

# 定义渲染器列表
if(WIN32)
    set(RENDERERS dx11 glsl spirv)
elseif(APPLE)
    set(RENDERERS metal glsl spirv)
else()
    set(RENDERERS glsl spirv)
endif()

# 编译所有着色器
set(ALL_SHADER_OUTPUTS "")

# 编译顶点着色器
foreach(SHADER ${VERTEX_SHADERS})
    foreach(RENDERER ${RENDERERS})
        compile_shader(${SHADER} VERTEX ${RENDERER} ${SHADER_OUTPUT_DIR})
        list(APPEND ALL_SHADER_OUTPUTS ${${SHADER}_${RENDERER}_OUTPUTS})
    endforeach()
endforeach()

# 编译片段着色器
foreach(SHADER ${FRAGMENT_SHADERS})
    foreach(RENDERER ${RENDERERS})
        compile_shader(${SHADER} FRAGMENT ${RENDERER} ${SHADER_OUTPUT_DIR})
        list(APPEND ALL_SHADER_OUTPUTS ${${SHADER}_${RENDERER}_OUTPUTS})
    endforeach()
endforeach()

# 编译计算着色器
foreach(SHADER ${COMPUTE_SHADERS})
    foreach(RENDERER ${RENDERERS})
        # 检查渲染器是否支持计算着色器
        set(COMPUTE_SUPPORTED TRUE)
        
        # GLSL需要OpenGL 4.3+才支持计算着色器
        if(${RENDERER} STREQUAL "glsl" AND APPLE)
            set(COMPUTE_SUPPORTED FALSE)  # macOS OpenGL 4.1不支持计算着色器
        endif()
        
        if(COMPUTE_SUPPORTED)
            compile_shader(${SHADER} COMPUTE ${RENDERER} ${SHADER_OUTPUT_DIR})
            list(APPEND ALL_SHADER_OUTPUTS ${${SHADER}_${RENDERER}_OUTPUTS})
        endif()
    endforeach()
endforeach()

# 创建自定义目标来编译所有着色器
add_custom_target(shaders ALL DEPENDS ${ALL_SHADER_OUTPUTS})

# ========================================
# 主程序配置
# ========================================

# 收集源文件
set(COMMON_SOURCE_FILES
    src/common/bgfx_utils.cpp
    src/common/camera.cpp
    src/common/cube_atlas.cpp
    src/common/example-glue.cpp
)

# 收集common下的子目录源文件
file(GLOB DEBUGDRAW_SOURCES src/common/debugdraw/*.cpp)
file(GLOB ENTRY_SOURCES src/common/entry/*.cpp)
file(GLOB FONT_SOURCES src/common/font/*.cpp)
file(GLOB IMGUI_SOURCES src/common/imgui/*.cpp)
file(GLOB NANOVG_SOURCES src/common/nanovg/*.cpp)
file(GLOB PS_SOURCES src/common/ps/*.cpp)

# 移除平台特定的entry文件，避免重复
if(WIN32)
    list(REMOVE_ITEM ENTRY_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_x11.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_osx.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_ios.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_android.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_asmjs.cpp
    )
elseif(APPLE)
    list(REMOVE_ITEM ENTRY_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_windows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_x11.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_android.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_asmjs.cpp
    )
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_osx.mm 
        PROPERTIES COMPILE_FLAGS "-x objective-c++"
    )
elseif(UNIX)
    list(REMOVE_ITEM ENTRY_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_windows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_osx.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_ios.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_android.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_asmjs.cpp
    )
endif()

# 创建主可执行文件
add_executable(${PROJECT_NAME}
    src/main.cpp
    ${COMMON_SOURCE_FILES}
    ${DEBUGDRAW_SOURCES}
    ${ENTRY_SOURCES}
    ${FONT_SOURCES}
    ${IMGUI_SOURCES}
    ${NANOVG_SOURCES}
    ${PS_SOURCES}
)

# 设置包含目录 - 添加第三方库路径
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bx/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bimg/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/3rdparty       # bgfx 第三方库根目录
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/3rdparty/dear-imgui  # Dear ImGui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/3rdparty/meshoptimizer  # meshoptimizer
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/3rdparty/stb  # STB libraries
    ${SHADER_INCLUDE_DIR}  # 包含生成的着色器头文件
)

# 如果 bgfx.cmake 有自己的第三方库位置，也添加这些路径
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/3rdparty)
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/3rdparty
        ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/3rdparty/dear-imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/3rdparty/meshoptimizer
        ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/3rdparty/stb
    )
endif()

# 平台特定的包含目录
if(WIN32)
    target_include_directories(${PROJECT_NAME} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bx/include/compat/msvc
    )
elseif(APPLE)
    target_include_directories(${PROJECT_NAME} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bx/include/compat/osx
    )
endif()

# 添加预处理器定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    # Entry 系统需要的定义
    ENTRY_CONFIG_IMPLEMENT_MAIN=1
    
    # 根据配置设置调试定义
    $<$<CONFIG:Debug>:BX_CONFIG_DEBUG=1>
    $<$<CONFIG:Release>:BX_CONFIG_DEBUG=0>
)

# 平台特定的预处理器定义
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _WIN32
        _WIN64
        _CRT_SECURE_NO_WARNINGS
        __STDC_FORMAT_MACROS
    )
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        __APPLE__
    )
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        __linux__
    )
endif()

# 链接bgfx库
target_link_libraries(${PROJECT_NAME}
    bgfx
    bimg
    bx
)

# 如果bgfx.cmake构建了第三方库，链接它们
if(TARGET dear-imgui)
    target_link_libraries(${PROJECT_NAME} dear-imgui)
endif()
if(TARGET meshoptimizer)
    target_link_libraries(${PROJECT_NAME} meshoptimizer)
endif()

# 确保着色器在主程序之前编译
add_dependencies(${PROJECT_NAME} shaders)

# Windows特定设置
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        psapi
    )
    # 注意：Windows GUI应用程序的入口点设置
    # 由于使用了 ENTRY_CONFIG_IMPLEMENT_MAIN，应该使用控制台子系统
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"
        LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE"
    )
endif()

# Linux特定设置
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    find_package(OpenGL REQUIRED)
    target_link_libraries(${PROJECT_NAME}
        ${X11_LIBRARIES}
        ${OPENGL_LIBRARIES}
        pthread
        dl
    )
endif()

# macOS特定设置
if(APPLE)
    find_library(COCOA_LIBRARY Cocoa)
    find_library(METAL_LIBRARY Metal)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    find_library(IOKIT_LIBRARY IOKit)
    target_link_libraries(${PROJECT_NAME}
        ${COCOA_LIBRARY}
        ${METAL_LIBRARY}
        ${QUARTZCORE_LIBRARY}
        ${IOKIT_LIBRARY}
    )
endif()

# 复制资源文件到构建目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/res $<TARGET_FILE_DIR:${PROJECT_NAME}>/../res
)

# 复制编译后的着色器到运行时目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${SHADER_OUTPUT_DIR} $<TARGET_FILE_DIR:${PROJECT_NAME}>/../res/runtime/shaders
)

# 设置VS调试器工作目录
if(MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY 
        VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    )
endif()

# 设置项目为启动项目（Visual Studio）
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY 
    VS_STARTUP_PROJECT ${PROJECT_NAME}
)

# 组织源文件（Visual Studio/Xcode）
source_group("Source Files" FILES src/main.cpp)
source_group("Source Files\\Common" FILES ${COMMON_SOURCE_FILES})
source_group("Source Files\\Common\\DebugDraw" FILES ${DEBUGDRAW_SOURCES})
source_group("Source Files\\Common\\Entry" FILES ${ENTRY_SOURCES})
source_group("Source Files\\Common\\Font" FILES ${FONT_SOURCES})
source_group("Source Files\\Common\\ImGui" FILES ${IMGUI_SOURCES})
source_group("Source Files\\Common\\NanoVG" FILES ${NANOVG_SOURCES})
source_group("Source Files\\Common\\PS" FILES ${PS_SOURCES})
source_group("Shaders\\Source" REGULAR_EXPRESSION ".*\\.sc$")
source_group("Shaders\\Include" REGULAR_EXPRESSION ".*\\.sh$")

# 输出配置信息
message(STATUS "========================================")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "Renderers: ${RENDERERS}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Shader output: ${SHADER_OUTPUT_DIR}")
message(STATUS "Shader headers: ${SHADER_INCLUDE_DIR}")
message(STATUS "========================================")