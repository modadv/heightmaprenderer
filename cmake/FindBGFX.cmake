# FindBGFX.cmake - 查找BGFX库

# 查找bgfx头文件
find_path(BGFX_INCLUDE_DIR
    NAMES bgfx/bgfx.h
    PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/external/bgfx.cmake/bgfx/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../external/bgfx.cmake/bgfx/include
    NO_DEFAULT_PATH
)

# 查找bgfx库文件
find_library(BGFX_LIBRARY
    NAMES bgfx
    PATHS
        ${CMAKE_CURRENT_BINARY_DIR}/external/bgfx.cmake/bgfx
        ${CMAKE_CURRENT_BINARY_DIR}/../external/bgfx.cmake/bgfx
    NO_DEFAULT_PATH
)

# 查找bimg库
find_library(BIMG_LIBRARY
    NAMES bimg
    PATHS
        ${CMAKE_CURRENT_BINARY_DIR}/external/bgfx.cmake/bimg
        ${CMAKE_CURRENT_BINARY_DIR}/../external/bgfx.cmake/bimg
    NO_DEFAULT_PATH
)

# 查找bx库
find_library(BX_LIBRARY
    NAMES bx
    PATHS
        ${CMAKE_CURRENT_BINARY_DIR}/external/bgfx.cmake/bx
        ${CMAKE_CURRENT_BINARY_DIR}/../external/bgfx.cmake/bx
    NO_DEFAULT_PATH
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(BGFX
    REQUIRED_VARS BGFX_INCLUDE_DIR BGFX_LIBRARY BIMG_LIBRARY BX_LIBRARY
)

if(BGFX_FOUND AND NOT TARGET BGFX::bgfx)
    add_library(BGFX::bgfx UNKNOWN IMPORTED)
    set_target_properties(BGFX::bgfx PROPERTIES
        IMPORTED_LOCATION "${BGFX_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${BGFX_INCLUDE_DIR}"
    )
endif()