cmake_minimum_required(VERSION 3.15)
project(cpct_interpreter LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(cpct
    cpct-interpreter/src/main.cpp
    cpct-interpreter/src/lexer.cpp
    cpct-interpreter/src/parser.cpp
    cpct-interpreter/src/interpreter.cpp
    cpct-interpreter/src/bigint.cpp
)

target_include_directories(cpct PRIVATE cpct-interpreter/src)

# exe는 프로젝트 루트에 출력
set_target_properties(cpct PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR}
)
