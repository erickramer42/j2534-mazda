add_library(proxy_dll SHARED
    proxy.cpp
    logger.cpp
    logger.h
    j2534_types.h
    exports.def
)

target_include_directories(proxy_dll PUBLIC ".")

set_target_properties(proxy_dll PROPERTIES
    OUTPUT_NAME "OBDXTrace"
    PREFIX ""
    SUFFIX ".dll"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/proxy"
)

# Copy to test/ as OBDXVX_J2534.dll for rig testing
add_custom_command(TARGET proxy_dll POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:proxy_dll>"
        "${CMAKE_SOURCE_DIR}/test/OBDXVX_J2534.dll"
    COMMENT "Copying OBDXTrace.dll to test/ for rig")
