cmake_minimum_required(VERSION 3.15)
project(cpct LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})

set(CORE_SRC src/core/lexer.cpp src/core/parser.cpp)
set(LIB_SRC src/lib/cpct/data/type/bigint.cpp)

# cpct.exe — unified entry point
add_executable(cpct src/main.cpp)

# cpct-translate.exe
add_executable(cpct-translate src/translate/main.cpp ${CORE_SRC} ${LIB_SRC})
target_include_directories(cpct-translate PRIVATE src/core)

# cpct-compile.exe
add_executable(cpct-compile src/compile/main.cpp)

# cpct-jit.exe
add_executable(cpct-jit src/jit/main.cpp ${CORE_SRC} ${LIB_SRC})
target_include_directories(cpct-jit PRIVATE src/core)
