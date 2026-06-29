cmake_minimum_required(VERSION 3.28.3)
project(ZeroKernel LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(kernel.wasm
    kernel/kernel.cpp
    kernel/hal_wasm.cpp
)

set_target_properties(kernel.wasm PROPERTIES SUFFIX "")
target_include_directories(kernel.wasm PRIVATE interfaces)

target_link_options(kernel.wasm PRIVATE
    "-sSTANDALONE_WASM=1"
    "-sEXPORTED_FUNCTIONS=_kernel_main,_hal_get,_handle_key,_handle_net_response"
    "-sERROR_ON_UNDEFINED_SYMBOLS=0"
    "-Wl,--no-entry"
    "-Wl,--import-undefined"
)