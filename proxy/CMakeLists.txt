cmake_minimum_required(VERSION 3.20)
project(j2534_proxy LANGUAGES CXX)

add_library(j2534_proxy SHARED
    proxy.cpp
    logger.cpp
)

# name must sync with .def file and registry entry FunctionLibrary path
set_target_properties(j2534_proxy PROPERTIES
    OUTPUT_NAME "OBDXTrace"
)

target_compile_features(j2534_proxy PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(j2534_proxy PRIVATE /W4)
endif()