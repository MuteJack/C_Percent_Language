cmake_minimum_required(VERSION 3.15)
project(cpct LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Output all executables to project root
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_SOURCE_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_SOURCE_DIR})

# Shared sources
set(CORE_SRC
    cpct-core/src/lexer.cpp
    cpct-core/src/parser.cpp
)
set(LIB_SRC
    cpct-cpp-lib/cpct/data/type/bigint.cpp
)

# ---- cpct-translate (C% → C++ translator) ----
add_executable(cpct-translate
    cpct-transpiler/translator/src/main.cpp
    ${CORE_SRC}
    ${LIB_SRC}
)
target_include_directories(cpct-translate PRIVATE cpct-core/src)

# ---- cpct-compile (translator + g++ compiler) ----
add_executable(cpct-compile
    cpct-transpiler/compiler/src/main.cpp
)

# ---- cpct-jit (Cling JIT REPL) ----
add_executable(cpct-jit
    cpct-jit/src/cling_repl.cpp
    ${CORE_SRC}
    ${LIB_SRC}
)
target_include_directories(cpct-jit PRIVATE cpct-core/src)
