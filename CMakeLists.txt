cmake_minimum_required(VERSION 3.20)
project(Nero LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
file(GLOB_RECURSE LIB_FILES "${SOURCE_DIR}/*.cpp")
foreach(_excl test.cpp benchmark.cpp wasm_api.cpp)
    list(REMOVE_ITEM LIB_FILES "${SOURCE_DIR}/${_excl}")
endforeach()

if(EMSCRIPTEN)
    file(GLOB_RECURSE WASM_FILES "${SOURCE_DIR}/*.cpp")
    foreach(_excl test.cpp benchmark.cpp)
        list(REMOVE_ITEM WASM_FILES "${SOURCE_DIR}/${_excl}")
    endforeach()

    add_executable(NeroWasm ${WASM_FILES})
    target_compile_options(NeroWasm PRIVATE -O3)
    target_link_options(NeroWasm PRIVATE
        -sWASM=1
        -sMODULARIZE=1
        -sEXPORT_ES6=0
        -sENVIRONMENT=web
        -sALLOW_MEMORY_GROWTH=1
        -sSTACK_SIZE=5242880
        -sFILESYSTEM=0
        -lembind
        --emit-tsd Nero.d.ts
    )
    set_target_properties(NeroWasm PROPERTIES OUTPUT_NAME "Nero" SUFFIX ".js")
    if(DEFINED ENV{EMSDK})
        target_include_directories(NeroWasm PRIVATE "$ENV{EMSDK}/upstream/emscripten/system/include/")
    else()
        target_include_directories(NeroWasm PRIVATE "/Users/illusion/dev/emsdk/upstream/emscripten/system/include/")
    endif()

else()
    add_library(NeroLib STATIC ${LIB_FILES})
    target_compile_options(NeroLib PRIVATE -Wall -O3 -Wno-reorder-init-list)
    target_include_directories(NeroLib PUBLIC "${SOURCE_DIR}")

    add_executable(NeroTest src/test.cpp)
    target_link_libraries(NeroTest PRIVATE NeroLib)
    target_compile_options(NeroTest PRIVATE -Wall -O3 -Wno-reorder-init-list)

    add_executable(NeroBenchmark src/benchmark.cpp)
    target_link_libraries(NeroBenchmark PRIVATE NeroLib)
    target_compile_options(NeroBenchmark PRIVATE -Wall -O3 -Wno-reorder-init-list)
endif()
