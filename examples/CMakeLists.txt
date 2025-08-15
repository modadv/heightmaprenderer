# Basic example
add_executable(basic_example basic_example.cpp)
target_link_libraries(basic_example PRIVATE heightmaprenderer::heightmaprenderer)

# Set output directory for basic example
set_target_properties(basic_example PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
    DEBUG_POSTFIX "_d"
)

# BGFX example (if enabled)
if(BGFX_AVAILABLE AND HEIGHTMAP_BUILD_BGFX_EXAMPLE)
    add_executable(bgfx_example bgfx_example.cpp)
    target_link_libraries(bgfx_example PRIVATE heightmaprenderer::heightmaprenderer)
    
    # Add BGFX libraries and include directories directly to bgfx_example
    target_link_libraries(bgfx_example PRIVATE bgfx bimg bx)
    
    # Add BGFX include directories
    target_include_directories(bgfx_example PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../external/bgfx.cmake/bgfx/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../external/bgfx.cmake/bimg/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../external/bgfx.cmake/bx/include
    )
    
    set_target_properties(bgfx_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        DEBUG_POSTFIX "_d"
    )
    
    message(STATUS "BGFX example will be built")
endif()

# Handle multi-config generators (Visual Studio, Xcode)
foreach(CONFIG Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set_target_properties(basic_example PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin
    )
    if(TARGET bgfx_example)
        set_target_properties(bgfx_example PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin
        )
    endif()
endforeach()

# Copy DLLs on Windows
if(WIN32 AND HEIGHTMAP_BUILD_SHARED_LIBS)
    add_custom_command(TARGET basic_example POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:heightmaprenderer>
        $<TARGET_FILE_DIR:basic_example>
    )
    
    if(TARGET bgfx_example)
        add_custom_command(TARGET bgfx_example POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:heightmaprenderer>
            $<TARGET_FILE_DIR:bgfx_example>
        )
    endif()
endif()

# Set RPATH for Unix systems
if(UNIX)
    set_target_properties(basic_example PROPERTIES
        BUILD_RPATH "${CMAKE_BINARY_DIR}/lib"
    )
    if(TARGET bgfx_example)
        set_target_properties(bgfx_example PROPERTIES
            BUILD_RPATH "${CMAKE_BINARY_DIR}/lib"
        )
    endif()
endif()