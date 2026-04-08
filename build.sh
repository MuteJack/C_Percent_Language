#!/usr/bin/env bash
# C% Build Script
# Usage:
#   bash build.sh              # Build all
#   bash build.sh translator   # Build translate.exe only
#   bash build.sh compiler     # Build compile.exe only
#   bash build.sh cpct         # Build cpct.exe only (unified CLI)

set -e

CXX="/mingw64/bin/g++"
CXXFLAGS="-std=c++20 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare"
CORE_SRC="cpct-core/src/lexer.cpp cpct-core/src/parser.cpp"
LIB_SRC=$(sed 's|^|cpct-cpp-lib/|' cpct-cpp-lib/sources.txt)

build_translator() {
    echo "Building translate.exe..."
    $CXX $CXXFLAGS \
        -I cpct-core/src \
        -o translate.exe \
        cpct-transpiler/translator/src/main.cpp \
        $CORE_SRC \
        $LIB_SRC
    echo "Build successful: translate.exe"
}

build_compiler() {
    echo "Building compile.exe..."
    $CXX $CXXFLAGS \
        -o compile.exe \
        cpct-transpiler/compiler/src/main.cpp
    echo "Build successful: compile.exe"
}

build_cpct() {
    echo "Building cpct.exe..."
    $CXX $CXXFLAGS \
        -I cpct-core/src \
        -o cpct.exe \
        cpct-jit/src/cling_repl.cpp \
        $CORE_SRC \
        $LIB_SRC
    echo "Build successful: cpct.exe"
}

case "${1:-all}" in
    translator) build_translator ;;
    compiler)   build_compiler ;;
    cpct)       build_cpct ;;
    all)
        build_translator
        build_compiler
        build_cpct
        ;;
    *)
        echo "Usage: bash build.sh [translator|compiler|cpct|all]"
        exit 1
        ;;
esac
