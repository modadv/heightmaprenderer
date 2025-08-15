# 这个文件用于复制DLL文件到调试目录

function(copy_dlls_for_debug target_name)
    if(WIN32 AND BUILD_SHARED_LIBS)
        # 获取所有链接的库
        get_target_property(LINKED_LIBRARIES ${target_name} LINK_LIBRARIES)
        
        foreach(lib ${LINKED_LIBRARIES})
            if(TARGET ${lib})
                get_target_property(lib_type ${lib} TYPE)
                if(lib_type STREQUAL "SHARED_LIBRARY")
                    add_custom_command(TARGET ${target_name} POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        $<TARGET_FILE:${lib}>
                        $<TARGET_FILE_DIR:${target_name}>
                        COMMENT "Copying ${lib} DLL"
                    )
                endif()
            endif()
        endforeach()
    endif()
endfunction()