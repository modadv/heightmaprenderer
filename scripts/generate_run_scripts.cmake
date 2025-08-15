# Generate run scripts for examples

# Ensure bin directory exists
file(MAKE_DIRECTORY "${BIN_DIR}")

if(WIN32)
    # Create basic example run script
    set(BASIC_SCRIPT_CONTENT 
"@echo off
set PATH=%~dp0;%PATH%
basic_example.exe %*
pause
")
    file(WRITE "${BIN_DIR}/run_basic_example.bat" "${BASIC_SCRIPT_CONTENT}")
    
    # Create BGFX example run script if target exists
    if(BUILD_BGFX_EXAMPLE)
        set(BGFX_SCRIPT_CONTENT 
"@echo off
set PATH=%~dp0;%PATH%
bgfx_example.exe %*
pause
")
        file(WRITE "${BIN_DIR}/run_bgfx_example.bat" "${BGFX_SCRIPT_CONTENT}")
    endif()
    
else()
    # Create basic example run script
    set(BASIC_SCRIPT_CONTENT 
"#!/bin/bash
DIR=\"$( cd \"$( dirname \"${BASH_SOURCE[0]}\" )\" && pwd )\"
export LD_LIBRARY_PATH=\"$DIR/../lib:$LD_LIBRARY_PATH\"
\"$DIR/basic_example\" \"$@\"
")
    file(WRITE "${BIN_DIR}/run_basic_example.sh" "${BASIC_SCRIPT_CONTENT}")
    
    # Make executable
    execute_process(
        COMMAND chmod +x "${BIN_DIR}/run_basic_example.sh"
        ERROR_QUIET
    )
    
    # Create BGFX example run script if target exists
    if(BUILD_BGFX_EXAMPLE)
        set(BGFX_SCRIPT_CONTENT 
"#!/bin/bash
DIR=\"$( cd \"$( dirname \"${BASH_SOURCE[0]}\" )\" && pwd )\"
export LD_LIBRARY_PATH=\"$DIR/../lib:$LD_LIBRARY_PATH\"
\"$DIR/bgfx_example\" \"$@\"
")
        file(WRITE "${BIN_DIR}/run_bgfx_example.sh" "${BGFX_SCRIPT_CONTENT}")
        
        # Make executable
        execute_process(
            COMMAND chmod +x "${BIN_DIR}/run_bgfx_example.sh"
            ERROR_QUIET
        )
    endif()
endif()

message(STATUS "Run scripts generated in ${BIN_DIR}")