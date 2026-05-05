cmake_minimum_required(VERSION 3.28.3)
project(ZeroKernel VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    kernel/kernel.cpp
)

add_executable(kernel.wasm ${SOURCES})

target_link_options(kernel.wasm PRIVATE
    "-nostdlib"
    "-s" "STANDALONE_WASM=1"
    "-s" "EXPORTED_FUNCTIONS=['_kernel_main']"
    "-Wl,--no-entry"
)