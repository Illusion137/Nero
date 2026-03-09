cmake_minimum_required(VERSION 3.20)
project(Nero LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SOURCE_DIR "${PROJECT_SOURCE_DIR}/src")
file(GLOB_RECURSE SRC_FILES "${SOURCE_DIR}/*.cpp")
list(REMOVE_ITEM SRC_FILES "${SOURCE_DIR}/main.cpp")

if(EMSCRIPTEN)
    add_executable(NeroWasm ${SRC_FILES})

    target_compile_options(NeroWasm PRIVATE
        -O3
    )

    target_link_options(NeroWasm PRIVATE
        -sWASM=1
        -sMODULARIZE=1
        -sEXPORT_ES6=0
        -sENVIRONMENT=web
        -sALLOW_MEMORY_GROWTH=1
        -sFILESYSTEM=0
        -lembind
        --emit-tsd Nero.d.ts
    )

    set_target_properties(NeroWasm PROPERTIES
        OUTPUT_NAME "Nero"
        SUFFIX ".js"
    )
    # target_include_directories(NeroWasm PRIVATE /Users/illusion/dev/emsdk/upstream/emscripten/system/include/)
else()
    # add_library(UnitEvalCore STATIC ${SRC_FILES})

    # target_compile_options(UnitEvalCore PRIVATE
    #     -Wall -O3 -Wno-reorder-init-list
    # )

    list(REMOVE_ITEM SRC_FILES "${SOURCE_DIR}/wasm_api.cpp")
    add_executable(
        Nero
        src/main.cpp
        ${SRC_FILES}
    )
    target_compile_options(Nero PRIVATE)
    # target_compile_options(Nero PRIVATE -Wall -O3 -Wno-reorder-init-list)
    # target_include_directories(Nero PRIVATE /Users/illusion/dev/emsdk/upstream/emscripten/system/include/)
endif()