cmake_minimum_required(VERSION 3.15)
project(heightmaprenderer VERSION 1.0.0 LANGUAGES CXX C)

# Set C++14 standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include modules
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Add cmake module path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Options
option(HEIGHTMAP_BUILD_EXAMPLES "Build examples" ON)
option(HEIGHTMAP_BUILD_SHARED_LIBS "Build shared libraries" ON)
option(HEIGHTMAP_INSTALL_EXAMPLES "Install examples" OFF)
option(HEIGHTMAP_WITH_BGFX "Build with BGFX support" ON)
option(HEIGHTMAP_BUILD_BGFX_EXAMPLE "Build BGFX example" ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Multi-config generator output directories
foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${CONFIG} CONFIG_UPPER)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib)
endforeach()

# Add bgfx submodule
if(HEIGHTMAP_WITH_BGFX)
    # Check if submodule exists
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/CMakeLists.txt")
        message(FATAL_ERROR 
            "BGFX submodule not found. Please run:\n"
            "git submodule update --init --recursive"
        )
    endif()

    # Set bgfx options
    set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
    set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "" FORCE)
    
    # Add bgfx subdirectory
    add_subdirectory(external/bgfx.cmake EXCLUDE_FROM_ALL)
    
    # Set bgfx target as available
    set(BGFX_AVAILABLE TRUE)
    message(STATUS "BGFX enabled and configured")
else()
    set(BGFX_AVAILABLE FALSE)
    message(STATUS "BGFX disabled")
endif()

# Set library type
if(HEIGHTMAP_BUILD_SHARED_LIBS)
    set(LIBRARY_TYPE SHARED)
else()
    set(LIBRARY_TYPE STATIC)
endif()

# Create library target
add_library(heightmaprenderer ${LIBRARY_TYPE}
    src/heightmap_renderer.cpp
)

# Set target properties
set_target_properties(heightmaprenderer PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
    DEBUG_POSTFIX "_d"
)

# Windows specific settings
if(WIN32)
    set_target_properties(heightmaprenderer PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
        ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    )
    
    # Set for multi-config generators
    foreach(CONFIG ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER ${CONFIG} CONFIG_UPPER)
        set_target_properties(heightmaprenderer PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin
            LIBRARY_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/bin
            ARCHIVE_OUTPUT_DIRECTORY_${CONFIG_UPPER} ${CMAKE_BINARY_DIR}/lib
        )
    endforeach()
endif()

# Include directories
target_include_directories(heightmaprenderer
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Compile definitions
if(HEIGHTMAP_BUILD_SHARED_LIBS)
    target_compile_definitions(heightmaprenderer
        PUBLIC HEIGHTMAP_SHARED_LIBS
        PRIVATE HEIGHTMAP_BUILDING_DLL
    )
endif()

# BGFX support - Use PRIVATE linking to avoid export issues
if(BGFX_AVAILABLE)
    target_compile_definitions(heightmaprenderer
        PUBLIC HEIGHTMAP_WITH_BGFX
    )
    # Use PRIVATE instead of PUBLIC to avoid export issues
    target_link_libraries(heightmaprenderer
        PRIVATE bgfx bimg bx
    )
endif()

# Compiler specific settings
if(MSVC)
    target_compile_options(heightmaprenderer PRIVATE /W4)
    if(HEIGHTMAP_BUILD_SHARED_LIBS)
        target_compile_definitions(heightmaprenderer PRIVATE _CRT_SECURE_NO_WARNINGS)
    endif()
else()
    target_compile_options(heightmaprenderer PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Create alias target
add_library(heightmaprenderer::heightmaprenderer ALIAS heightmaprenderer)

# Install targets - Only install our library, not BGFX dependencies
install(TARGETS heightmaprenderer
    EXPORT heightmaprendererTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install header files
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

# Export targets - Only export our targets
install(EXPORT heightmaprendererTargets
    FILE heightmaprendererTargets.cmake
    NAMESPACE heightmaprenderer::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/heightmaprenderer
)

# Configure package config file
configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/heightmaprendererConfig.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/heightmaprendererConfig.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/heightmaprenderer
)

# Version config file
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/heightmaprendererConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install config files
install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/heightmaprendererConfig.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/heightmaprendererConfigVersion.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/heightmaprenderer
)

# Build examples
if(HEIGHTMAP_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Generate run scripts after configuration
if(HEIGHTMAP_BUILD_EXAMPLES)
    # Set variables for script generation
    set(BIN_DIR "${CMAKE_BINARY_DIR}/bin")
    set(BUILD_BGFX_EXAMPLE ${HEIGHTMAP_BUILD_BGFX_EXAMPLE})
    
    # Execute script generation
    execute_process(
        COMMAND ${CMAKE_COMMAND}
            -DBIN_DIR=${BIN_DIR}
            -DBUILD_BGFX_EXAMPLE=${BUILD_BGFX_EXAMPLE}
            -P ${CMAKE_CURRENT_SOURCE_DIR}/scripts/generate_run_scripts.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        ERROR_QUIET
    )
endif()

# Display configuration information
message(STATUS "HeightmapRenderer Configuration:")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Shared libs: ${HEIGHTMAP_BUILD_SHARED_LIBS}")
message(STATUS "  Build examples: ${HEIGHTMAP_BUILD_EXAMPLES}")
message(STATUS "  BGFX support: ${BGFX_AVAILABLE}")
if(BGFX_AVAILABLE)
    message(STATUS "  Build BGFX example: ${HEIGHTMAP_BUILD_BGFX_EXAMPLE}")
endif()