#!/usr/bin/env bash
# C% Build Script
# Usage:
#   bash build.sh              # Build all
#   bash build.sh translator   # Build cpct-translate.exe only
#   bash build.sh compiler     # Build cpct-compile.exe only
#   bash build.sh jit          # Build cpct-jit.exe only
#   bash build.sh cpct         # Build cpct.exe only

set -e

CXX="/mingw64/bin/g++"
CXXFLAGS="-std=c++20 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare"
CORE_SRC="src/core/lexer.cpp src/core/parser.cpp"
LIB_SRC=$(sed 's|^|src/lib/|' src/lib/sources.txt)

build_translator() {
    echo "Building cpct-translate.exe..."
    $CXX $CXXFLAGS \
        -I src/core \
        -o cpct-translate.exe \
        src/translate/main.cpp \
        $CORE_SRC \
        $LIB_SRC
    echo "Build successful: cpct-translate.exe"
}

build_compiler() {
    echo "Building cpct-compile.exe..."
    $CXX $CXXFLAGS \
        -o cpct-compile.exe \
        src/compile/main.cpp
    echo "Build successful: cpct-compile.exe"
}

build_jit() {
    echo "Building cpct-jit.exe..."
    $CXX $CXXFLAGS \
        -I src/core \
        -o cpct-jit.exe \
        src/jit/main.cpp \
        $CORE_SRC \
        $LIB_SRC
    echo "Build successful: cpct-jit.exe"
}

build_cpct() {
    echo "Building cpct.exe..."
    $CXX $CXXFLAGS \
        -o cpct.exe \
        src/main.cpp
    echo "Build successful: cpct.exe"
}

case "${1:-all}" in
    translator) build_translator ;;
    compiler)   build_compiler ;;
    jit)        build_jit ;;
    cpct)       build_cpct ;;
    all)
        build_translator
        build_compiler
        build_jit
        build_cpct
        ;;
    *)
        echo "Usage: bash build.sh [translator|compiler|jit|cpct|all]"
        exit 1
        ;;
esac
