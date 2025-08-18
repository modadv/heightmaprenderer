# ========================================
# CMake 基础配置
# ========================================

# 指定 CMake 的最低版本要求
# 3.14 版本支持较新的特性，如更好的 MSVC 运行时库处理
cmake_minimum_required(VERSION 3.14)

# 定义项目名称和使用的编程语言
# project() 命令会自动创建一些变量，如 PROJECT_NAME, PROJECT_SOURCE_DIR 等
project(heightmaprenderer)

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
add_subdirectory(external/bgfx.cmake)

# ========================================
# 添加terrain-renderer库
# ========================================
add_subdirectory(lib)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/src
    # 项目的着色器目录（包含自定义的 .sh 文件）
    ${SHADER_SOURCE_DIR}
    # common 目录（包含 common.sh 和 shaderlib.sh）
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    # bgfx examples 的 common 目录（可能包含额外的辅助着色器）
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/examples/common
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

# 定义通用源文件列表（所有平台都需要的文件）
set(COMMON_SOURCE_FILES
    src/common/bgfx_utils.cpp       # bgfx 实用工具函数
    src/common/camera.cpp            # 相机控制
    src/common/cube_atlas.cpp        # 立方体纹理图集
    src/common/example-glue.cpp     # 示例程序粘合代码
)

# 使用 GLOB 收集子目录中的所有源文件
# file(GLOB ...) 会查找匹配模式的所有文件
file(GLOB DEBUGDRAW_SOURCES src/common/debugdraw/*.cpp)    # 调试绘制
file(GLOB ENTRY_SOURCES src/common/entry/*.cpp)            # 程序入口
file(GLOB FONT_SOURCES src/common/font/*.cpp)              # 字体渲染
file(GLOB IMGUI_SOURCES src/common/imgui/*.cpp)            # Dear ImGui 界面
file(GLOB NANOVG_SOURCES src/common/nanovg/*.cpp)          # NanoVG 矢量图形
file(GLOB PS_SOURCES src/common/ps/*.cpp)                  # 粒子系统

# 移除平台特定的 entry 文件，避免编译错误的平台代码
if(WIN32)
    # Windows 平台：移除其他平台的入口文件
    list(REMOVE_ITEM ENTRY_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_x11.cpp      # Linux X11
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_osx.mm      # macOS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_ios.mm      # iOS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_android.cpp # Android
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_asmjs.cpp   # Emscripten
    )
elseif(APPLE)
    # macOS 平台：移除其他平台的入口文件
    list(REMOVE_ITEM ENTRY_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_windows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_x11.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_android.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_asmjs.cpp
    )
    # 设置 Objective-C++ 编译标志（.mm 文件）
    set_source_files_properties(
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_osx.mm 
        PROPERTIES COMPILE_FLAGS "-x objective-c++"
    )
elseif(UNIX)
    # Linux 平台：移除其他平台的入口文件
    list(REMOVE_ITEM ENTRY_SOURCES 
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_windows.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_osx.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_ios.mm
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_android.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common/entry/entry_asmjs.cpp
    )
endif()

# ========================================
# 创建可执行文件目标
# ========================================

# add_executable 创建可执行文件目标
add_executable(${PROJECT_NAME}
    src/main.cpp                    # 主程序文件
    ${COMMON_SOURCE_FILES}           # 通用源文件
    ${DEBUGDRAW_SOURCES}             # 调试绘制源文件
    ${ENTRY_SOURCES}                 # 平台入口源文件
    ${FONT_SOURCES}                  # 字体源文件
    ${IMGUI_SOURCES}                 # ImGui 源文件
    ${NANOVG_SOURCES}                # NanoVG 源文件
    ${PS_SOURCES}                    # 粒子系统源文件
)

# ========================================
# 设置包含目录（头文件搜索路径）
# ========================================

# target_include_directories 设置目标的包含目录
# PRIVATE 表示这些目录只对当前目标可见
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src                    # 项目源代码目录
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common             # common 目录
    # bgfx 相关的包含目录
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/include      # bgfx 公共头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bx/include        # bx 工具库头文件
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bimg/include      # bimg 图像库头文件
    # 第三方库目录
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/3rdparty     # bgfx 第三方库根目录
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/3rdparty/dear-imgui   # Dear ImGui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/3rdparty/meshoptimizer # 网格优化器
    ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/3rdparty/stb          # STB 单头文件库
    ${SHADER_INCLUDE_DIR}  # 生成的着色器头文件目录
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
    # Windows 平台需要 MSVC 兼容性头文件
    target_include_directories(${PROJECT_NAME} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bx/include/compat/msvc
    )
elseif(APPLE)
    # macOS 平台需要 OSX 兼容性头文件
    target_include_directories(${PROJECT_NAME} PRIVATE 
        ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bx/include/compat/osx
    )
endif()

# ========================================
# 设置预处理器定义（编译时宏）
# ========================================

# target_compile_definitions 设置编译时的宏定义
target_compile_definitions(${PROJECT_NAME} PRIVATE
    # Entry 系统需要这个宏来正确实现 main 函数
    ENTRY_CONFIG_IMPLEMENT_MAIN=1
    
    # 根据构建配置设置调试宏
    # $<$<CONFIG:Debug>:...> 是生成器表达式，只在 Debug 配置时生效
    $<$<CONFIG:Debug>:BX_CONFIG_DEBUG=1>
    $<$<CONFIG:Release>:BX_CONFIG_DEBUG=0>
)

# 平台特定的预处理器定义
if(WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        _WIN32                          # Windows 32 位标识（64 位系统也定义）
        _WIN64                          # Windows 64 位标识
        _CRT_SECURE_NO_WARNINGS         # 禁用 MSVC 的安全警告
        __STDC_FORMAT_MACROS            # 启用 C99 格式宏
    )
elseif(APPLE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        __APPLE__                       # Apple 平台标识
    )
elseif(UNIX)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        __linux__                       # Linux 平台标识
    )
endif()

# ========================================
# 链接库
# ========================================

# target_link_libraries 指定要链接的库
target_link_libraries(${PROJECT_NAME}
    bgfx    # bgfx 图形库
    bimg    # bimg 图像处理库
    bx      # bx 基础工具库
    terrain-renderer  # 高度图/地形渲染
)

# 如果 bgfx.cmake 构建了第三方库目标，链接它们
if(TARGET dear-imgui)
    target_link_libraries(${PROJECT_NAME} dear-imgui)
endif()
if(TARGET meshoptimizer)
    target_link_libraries(${PROJECT_NAME} meshoptimizer)
endif()

# 确保着色器在主程序之前编译
# add_dependencies 创建目标间的依赖关系
add_dependencies(${PROJECT_NAME} shaders)

# ========================================
# 平台特定的链接配置
# ========================================

if(WIN32)
    # Windows 平台需要链接的系统库
    target_link_libraries(${PROJECT_NAME}
        psapi    # Process Status API，用于进程信息
    )
    # 设置链接器标志
    # 使用控制台子系统（entry 系统会创建窗口）
    set_target_properties(${PROJECT_NAME} PROPERTIES 
        LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE"      # Debug 使用控制台
        LINK_FLAGS_RELEASE "/SUBSYSTEM:CONSOLE"    # Release 也使用控制台
    )
endif()

if(UNIX AND NOT APPLE)
    # Linux 平台需要的库
    find_package(X11 REQUIRED)          # X11 窗口系统
    find_package(OpenGL REQUIRED)       # OpenGL 图形库
    target_link_libraries(${PROJECT_NAME}
        ${X11_LIBRARIES}                # X11 库
        ${OPENGL_LIBRARIES}             # OpenGL 库
        pthread                          # POSIX 线程库
        dl                               # 动态链接库
    )
endif()

if(APPLE)
    # macOS 平台需要的框架
    find_library(COCOA_LIBRARY Cocoa)           # Cocoa 框架（窗口和事件）
    find_library(METAL_LIBRARY Metal)           # Metal 图形框架
    find_library(QUARTZCORE_LIBRARY QuartzCore) # Core Animation 框架
    find_library(IOKIT_LIBRARY IOKit)           # I/O Kit 框架
    target_link_libraries(${PROJECT_NAME}
        ${COCOA_LIBRARY}
        ${METAL_LIBRARY}
        ${QUARTZCORE_LIBRARY}
        ${IOKIT_LIBRARY}
    )
endif()

# ========================================
# 资源文件复制配置
# ========================================

# 使用 add_custom_command 在构建后复制资源文件
# POST_BUILD 表示在目标构建完成后执行

# 复制 runtime 目录的所有内容到 bin 目录
# 这包括字体、图像、网格、着色器、纹理等运行时资源
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/res/runtime $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying runtime contents to bin directory..."
)

# 复制编译后的着色器，覆盖默认的着色器文件
foreach(RENDERER ${RENDERERS})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${SHADER_OUTPUT_DIR}/${RENDERER} $<TARGET_FILE_DIR:${PROJECT_NAME}>/shaders/${RENDERER}
        COMMENT "Copying compiled ${RENDERER} shaders..."
    )
endforeach()

# ========================================
# Visual Studio 特定配置
# ========================================

if(MSVC)
    # 设置 VS 调试器的工作目录
    # 这确保在 VS 中按 F5 调试时，程序能找到资源文件
    set_property(TARGET ${PROJECT_NAME} PROPERTY 
        VS_DEBUGGER_WORKING_DIRECTORY $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()

# 设置项目为 VS 的启动项目
# 这样打开解决方案时，heightmaprenderer 会被设为默认启动项目
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY 
    VS_STARTUP_PROJECT ${PROJECT_NAME}
)

# ========================================
# 源文件组织（IDE 中的文件夹结构）
# ========================================

# source_group 在 IDE（如 Visual Studio）中创建虚拟文件夹
# 这不影响编译，只是为了更好地组织文件显示
source_group("Source Files" FILES src/main.cpp)
source_group("Source Files\\Common" FILES ${COMMON_SOURCE_FILES})
source_group("Source Files\\Common\\DebugDraw" FILES ${DEBUGDRAW_SOURCES})
source_group("Source Files\\Common\\Entry" FILES ${ENTRY_SOURCES})
source_group("Source Files\\Common\\Font" FILES ${FONT_SOURCES})
source_group("Source Files\\Common\\ImGui" FILES ${IMGUI_SOURCES})
source_group("Source Files\\Common\\NanoVG" FILES ${NANOVG_SOURCES})
source_group("Source Files\\Common\\PS" FILES ${PS_SOURCES})
# 使用正则表达式匹配着色器文件
source_group("Shaders\\Source" REGULAR_EXPRESSION ".*\\.sc$")
source_group("Shaders\\Include" REGULAR_EXPRESSION ".*\\.sh$")

# ========================================
# 安装配置（用于打包发布）
# ========================================

# 设置安装目录前缀
set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/install CACHE PATH "Installation Directory")

# 安装可执行文件到根目录
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION .
)

# 安装 runtime 目录的所有内容
install(DIRECTORY ${CMAKE_SOURCE_DIR}/res/runtime/
    DESTINATION .
)

# 安装编译后的着色器（覆盖默认的）
foreach(RENDERER ${RENDERERS})
    if(EXISTS ${SHADER_OUTPUT_DIR}/${RENDERER})
        install(DIRECTORY ${SHADER_OUTPUT_DIR}/${RENDERER}/
            DESTINATION shaders/${RENDERER}
        )
    endif()
endforeach()

# ========================================
# 配置信息输出
# ========================================

# 输出配置信息，帮助调试和确认设置
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

# 在主CMakeLists.txt的末尾添加
# ========================================
# 库测试程序（可选）
# ========================================
option(BUILD_TERRAIN_LIB_TESTS "Build terrain library tests" ON)
# ========================================
# 库测试程序（可选）
# ========================================
option(BUILD_TERRAIN_LIB_TESTS "Build terrain library tests" ON)

if(BUILD_TERRAIN_LIB_TESTS)
    # 01 - 基础测试
    add_executable(01_test_basic examples/01_test_basic.cpp)
    target_link_libraries(01_test_basic 
        terrain-renderer
        bgfx bimg bx
    )
    # 添加包含路径
    target_include_directories(01_test_basic PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/include
    )
    if(WIN32)
        target_link_libraries(01_test_basic psapi)
    endif()
    set_target_properties(01_test_basic PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    
    # 02 - 渲染上下文测试
    add_executable(02_test_render examples/02_test_render.cpp)
    target_link_libraries(02_test_render 
        terrain-renderer
        bgfx bimg bx
    )
    target_include_directories(02_test_render PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/include
    )
    if(WIN32)
        target_link_libraries(02_test_render psapi)
    endif()
    set_target_properties(02_test_render PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    
    # 03 - 高度图加载测试
    add_executable(03_test_heightmap examples/03_test_heightmap.cpp)
    target_link_libraries(03_test_heightmap 
        terrain-renderer
        bgfx bimg bx
    )
    target_include_directories(03_test_heightmap PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/include
    )
    if(WIN32)
        target_link_libraries(03_test_heightmap psapi)
    endif()
    set_target_properties(03_test_heightmap PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    
    # 复制测试需要的纹理文件
    add_custom_command(TARGET 03_test_heightmap POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/res/runtime/textures 
        $<TARGET_FILE_DIR:03_test_heightmap>/textures
        COMMENT "Copying textures for heightmap test..."
    )

    # 04 - 可视化测试
    add_executable(04_test_visual examples/04_test_visual.cpp)
    target_link_libraries(04_test_visual 
        terrain-renderer
        bgfx bimg bx
    )
    target_include_directories(04_test_visual PRIVATE
        ${CMAKE_SOURCE_DIR}/lib/include
    )
    if(WIN32)
        target_link_libraries(04_test_visual psapi)
    endif()
    set_target_properties(04_test_visual PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    )
    
    # 复制纹理文件
    add_custom_command(TARGET 04_test_visual POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/res/runtime/textures 
        $<TARGET_FILE_DIR:04_test_visual>/textures
        COMMENT "Copying textures for visual test..."
    )
endif()