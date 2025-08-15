# 基础示例
add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example PRIVATE heightmaprenderer::heightmaprenderer)

# 设置输出目录
set_target_properties(basic_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    DEBUG_POSTFIX "_d"
)

# 多配置生成器的输出目录
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set_target_properties(basic_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin
    )
endforeach()

# Windows DLL复制
if(WIN32 AND HEIGHTMAP_BUILD_SHARED_LIBS)
    # 复制DLL到可执行文件目录
    add_custom_command(TARGET basic_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:heightmaprenderer>
        $<TARGET_FILE_DIR:basic_example>
        COMMENT "Copying heightmaprenderer DLL to example directory"
    )
endif()

# 设置运行时路径（Linux/macOS）
if(UNIX)
    set_target_properties(basic_example PROPERTIES
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
        BUILD_RPATH "${CMAKE_BINARY_DIR}/lib"
    )
endif()

# 安装示例（可选）
if(HEIGHTMAP_INSTALL_EXAMPLES)
    install(TARGETS basic_example
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    
    # Windows下同时安装DLL
    if(WIN32 AND HEIGHTMAP_BUILD_SHARED_LIBS)
        install(FILES $<TARGET_FILE:heightmaprenderer>
            DESTINATION ${CMAKE_INSTALL_BINDIR}
        )
    endif()
endif()

# 创建运行脚本
if(WIN32)
    set(SCRIPT_CONTENT "@echo off\nset PATH=%~dp0;%PATH%\nbasic_example.exe %*\n")
    file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/bin/run_example.bat CONTENT ${SCRIPT_CONTENT})
else()
    set(SCRIPT_CONTENT "#!/bin/bash\nDIR=\"$( cd \"$( dirname \"${BASH_SOURCE[0]}\" )\" && pwd )\"\nexport LD_LIBRARY_PATH=\"$DIR/../lib:$LD_LIBRARY_PATH\"\n\"$DIR/basic_example\" \"$@\"\n")
    file(GENERATE OUTPUT ${CMAKE_BINARY_DIR}/bin/run_example.sh CONTENT ${SCRIPT_CONTENT})
    # 设置执行权限
    file(COPY ${CMAKE_BINARY_DIR}/bin/run_example.sh
         DESTINATION ${CMAKE_BINARY_DIR}/bin
         FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()