#!/usr/bin/env bash
# C% Build Script
# Usage:
#   bash build.sh              # Build all
#   bash build.sh translator   # Build translator only
#   bash build.sh compiler     # Build compiler only
#   bash build.sh jit          # Build Cling JIT REPL only

set -e

CXX="/mingw64/bin/g++"
CXXFLAGS="-std=c++20 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare"
CORE_SRC="cpct-core/src/lexer.cpp cpct-core/src/parser.cpp"
LIB_SRC=$(sed 's|^|cpct-cpp-lib/|' cpct-cpp-lib/sources.txt)

build_translator() {
    echo "Building C% Translator..."
    $CXX $CXXFLAGS \
        -I cpct-core/src \
        -o cpct-translate.exe \
        cpct-transpiler/translator/src/main.cpp \
        $CORE_SRC \
        $LIB_SRC
    echo "Build successful: cpct-translate.exe"
}

build_compiler() {
    echo "Building C% Compiler..."
    $CXX $CXXFLAGS \
        -o cpct-compile.exe \
        cpct-transpiler/compiler/src/main.cpp
    echo "Build successful: cpct-compile.exe"
}

build_jit() {
    echo "Building C% Cling JIT REPL..."
    $CXX $CXXFLAGS \
        -I cpct-core/src \
        -o cpct-jit.exe \
        cpct-jit/src/cling_repl.cpp \
        $CORE_SRC \
        $LIB_SRC
    echo "Build successful: cpct-jit.exe"
}

case "${1:-all}" in
    translator) build_translator ;;
    compiler)   build_compiler ;;
    jit)        build_jit ;;
    all)
        build_translator
        build_compiler
        build_jit
        ;;
    *)
        echo "Usage: bash build.sh [translator|compiler|jit|all]"
        exit 1
        ;;
esac
