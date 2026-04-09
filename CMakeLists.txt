cmake_minimum_required(VERSION 3.15)
project(cpct LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})

# Git commit hash for nightly builds
execute_process(
    COMMAND git rev-parse --short HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT GIT_HASH)
    set(GIT_HASH "unknown")
endif()
string(TIMESTAMP BUILD_TIME "%Y%m%d-%H%M%S")
add_compile_definitions(CPCT_GIT_HASH="${GIT_HASH}" CPCT_BUILD_TIME="${BUILD_TIME}")

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
if(UNIX)
    find_package(Threads REQUIRED)
    target_link_libraries(cpct-jit PRIVATE Threads::Threads)
endif()
