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

# shaderc工具将作为目标生成，获取其路径
if(WIN32)
    set(SHADERC_SUFFIX ".exe")
else()
    set(SHADERC_SUFFIX "")
endif()

# 设置着色器目录
set(SHADER_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/shaders)
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/shaders)
set(SHADER_INCLUDE_DIR ${CMAKE_BINARY_DIR}/include/generated/shaders)

# 创建输出目录
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})
file(MAKE_DIRECTORY ${SHADER_INCLUDE_DIR})

# 定义平台和渲染器配置
if(WIN32)
    set(SHADER_PLATFORM windows)
    set(SHADER_PROFILES
        "s_5_0:dx11"    # DirectX 11
        "430:glsl"      # OpenGL
        "spirv:spirv"   # Vulkan
    )
elseif(APPLE)
    set(SHADER_PLATFORM osx)
    set(SHADER_PROFILES
        "metal:metal"   # Metal
        "410:glsl"      # OpenGL
        "spirv:spirv"   # Vulkan/MoltenVK
    )
elseif(UNIX)
    set(SHADER_PLATFORM linux)
    set(SHADER_PROFILES
        "430:glsl"      # OpenGL
        "spirv:spirv"   # Vulkan
    )
endif()

# varying.def.sc文件路径
set(VARYING_DEF ${SHADER_SOURCE_DIR}/varying.def.sc)

# 着色器包含路径 - 添加bgfx的着色器包含目录
set(SHADER_INCLUDE_PATHS
    ${SHADER_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/src  # bgfx内部着色器包含文件
)

# 构建包含路径参数
set(INCLUDE_ARGS "")
foreach(INCLUDE_PATH ${SHADER_INCLUDE_PATHS})
    list(APPEND INCLUDE_ARGS -i ${INCLUDE_PATH})
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
function(compile_shader SHADER_NAME SHADER_TYPE PROFILE OUTPUT_DIR)
    string(REPLACE ":" ";" PROFILE_PARTS ${PROFILE})
    list(GET PROFILE_PARTS 0 PROFILE_VERSION)
    list(GET PROFILE_PARTS 1 PROFILE_RENDERER)
    
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
    
    # 输入和输出文件路径
    set(INPUT_FILE ${SHADER_SOURCE_DIR}/${SHADER_NAME}.sc)
    set(OUTPUT_BIN ${OUTPUT_DIR}/${PROFILE_RENDERER}/${SHADER_NAME}.bin)
    set(OUTPUT_HEADER ${SHADER_INCLUDE_DIR}/${PROFILE_RENDERER}/${SHADER_NAME}.bin.h)
    
    # 创建渲染器特定的输出目录
    file(MAKE_DIRECTORY ${OUTPUT_DIR}/${PROFILE_RENDERER})
    file(MAKE_DIRECTORY ${SHADER_INCLUDE_DIR}/${PROFILE_RENDERER})
    
    # 生成二进制文件的命令 - 使用shaderc目标
    add_custom_command(
        OUTPUT ${OUTPUT_BIN}
        COMMAND $<TARGET_FILE:shaderc>
            -f ${INPUT_FILE}
            -o ${OUTPUT_BIN}
            --type ${TYPE_FLAG}
            --platform ${SHADER_PLATFORM}
            --profile ${PROFILE_VERSION}
            ${INCLUDE_ARGS}
            $<$<NOT:$<STREQUAL:${SHADER_TYPE},COMPUTE>>:--varyingdef ${VARYING_DEF}>
            $<$<CONFIG:Debug>:--debug>
            $<$<CONFIG:Release>:-O 3>
        DEPENDS ${INPUT_FILE} ${VARYING_DEF} shaderc
        COMMENT "Compiling ${SHADER_TYPE} shader ${SHADER_NAME} for ${PROFILE_RENDERER}"
        VERBATIM
    )
    
    # 生成C头文件的命令 - 使用shaderc目标
    add_custom_command(
        OUTPUT ${OUTPUT_HEADER}
        COMMAND $<TARGET_FILE:shaderc>
            -f ${INPUT_FILE}
            -o ${OUTPUT_HEADER}
            --type ${TYPE_FLAG}
            --platform ${SHADER_PLATFORM}
            --profile ${PROFILE_VERSION}
            --bin2c ${SHADER_NAME}_${PROFILE_RENDERER}
            ${INCLUDE_ARGS}
            $<$<NOT:$<STREQUAL:${SHADER_TYPE},COMPUTE>>:--varyingdef ${VARYING_DEF}>
            $<$<CONFIG:Debug>:--debug>
            $<$<CONFIG:Release>:-O 3>
        DEPENDS ${INPUT_FILE} ${VARYING_DEF} shaderc
        COMMENT "Generating header for ${SHADER_TYPE} shader ${SHADER_NAME} for ${PROFILE_RENDERER}"
        VERBATIM
    )
    
    # 返回生成的文件列表
    set(${SHADER_NAME}_${PROFILE_RENDERER}_OUTPUTS 
        ${OUTPUT_BIN} 
        ${OUTPUT_HEADER} 
        PARENT_SCOPE
    )
endfunction()

# 编译所有着色器
set(ALL_SHADER_OUTPUTS "")

# 编译顶点着色器
foreach(SHADER ${VERTEX_SHADERS})
    foreach(PROFILE ${SHADER_PROFILES})
        compile_shader(${SHADER} VERTEX ${PROFILE} ${SHADER_OUTPUT_DIR})
        string(REPLACE ":" "_" PROFILE_VAR ${PROFILE})
        string(REPLACE ":" ";" PROFILE_PARTS ${PROFILE})
        list(GET PROFILE_PARTS 1 RENDERER)
        list(APPEND ALL_SHADER_OUTPUTS ${${SHADER}_${RENDERER}_OUTPUTS})
    endforeach()
endforeach()

# 编译片段着色器
foreach(SHADER ${FRAGMENT_SHADERS})
    foreach(PROFILE ${SHADER_PROFILES})
        compile_shader(${SHADER} FRAGMENT ${PROFILE} ${SHADER_OUTPUT_DIR})
        string(REPLACE ":" "_" PROFILE_VAR ${PROFILE})
        string(REPLACE ":" ";" PROFILE_PARTS ${PROFILE})
        list(GET PROFILE_PARTS 1 RENDERER)
        list(APPEND ALL_SHADER_OUTPUTS ${${SHADER}_${RENDERER}_OUTPUTS})
    endforeach()
endforeach()

# 编译计算着色器
foreach(SHADER ${COMPUTE_SHADERS})
    foreach(PROFILE ${SHADER_PROFILES})
        # 检查profile是否支持计算着色器
        string(REGEX MATCH "^(s_5_0|430|spirv|metal)" COMPUTE_SUPPORTED ${PROFILE})
        if(COMPUTE_SUPPORTED)
            compile_shader(${SHADER} COMPUTE ${PROFILE} ${SHADER_OUTPUT_DIR})
            string(REPLACE ":" "_" PROFILE_VAR ${PROFILE})
            string(REPLACE ":" ";" PROFILE_PARTS ${PROFILE})
            list(GET PROFILE_PARTS 1 RENDERER)
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

# 设置包含目录
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bx/include
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bimg/include
    ${SHADER_INCLUDE_DIR}  # 包含生成的着色器头文件
)

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

# 链接bgfx库
target_link_libraries(${PROJECT_NAME}
    bgfx
    bimg
    bx
)

# 如果需要使用bgfx的example-common库（如果它被构建）
if(TARGET example-common)
    target_link_libraries(${PROJECT_NAME} example-common)
endif()

# 确保着色器在主程序之前编译
add_dependencies(${PROJECT_NAME} shaders)

# Windows特定设置
if(WIN32)
    target_link_libraries(${PROJECT_NAME}
        psapi
    )
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup"
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
message(STATUS "Shader platform: ${SHADER_PLATFORM}")
message(STATUS "Shader profiles: ${SHADER_PROFILES}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Output directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "Shader output: ${SHADER_OUTPUT_DIR}")
message(STATUS "Shader headers: ${SHADER_INCLUDE_DIR}")
message(STATUS "========================================")