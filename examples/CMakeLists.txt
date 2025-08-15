# 基础示例
add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example PRIVATE heightmaprenderer::heightmaprenderer)

# 设置运行时路径（Linux/macOS）
if(UNIX)
    set_target_properties(basic_example PROPERTIES
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endif()

# 安装示例（可选）
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    install(TARGETS basic_example
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
endif()