#!/usr/bin/env bash
# C% Build Script (cross-platform: Windows MSYS2, Linux, macOS)
# Usage:
#   bash build.sh              # Build all
#   bash build.sh translator   # Build cpct-translate only
#   bash build.sh compiler     # Build cpct-compile only
#   bash build.sh jit          # Build cpct-jit only
#   bash build.sh cpct         # Build cpct only

set -e

# Platform detection
if [[ "$OSTYPE" == "msys" || "$OSTYPE" == "mingw"* || "$OSTYPE" == "cygwin" ]]; then
    CXX="/mingw64/bin/g++"
    EXT=".exe"
else
    CXX="g++"
    EXT=""
fi

CXXFLAGS="-std=c++20 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare"
CORE_SRC="src/core/lexer.cpp src/core/parser.cpp"
LIB_SRC=$(sed 's|^|src/lib/|' src/lib/sources.txt)

build_translator() {
    echo "Building cpct-translate${EXT}..."
    $CXX $CXXFLAGS \
        -I src/core \
        -o "cpct-translate${EXT}" \
        src/translate/main.cpp \
        $CORE_SRC \
        $LIB_SRC
    echo "Build successful: cpct-translate${EXT}"
}

build_compiler() {
    echo "Building cpct-compile${EXT}..."
    $CXX $CXXFLAGS \
        -o "cpct-compile${EXT}" \
        src/compile/main.cpp
    echo "Build successful: cpct-compile${EXT}"
}

build_jit() {
    echo "Building cpct-jit${EXT}..."
    $CXX $CXXFLAGS \
        -I src/core \
        -o "cpct-jit${EXT}" \
        src/jit/main.cpp \
        $CORE_SRC \
        $LIB_SRC
    echo "Build successful: cpct-jit${EXT}"
}

build_cpct() {
    echo "Building cpct${EXT}..."
    $CXX $CXXFLAGS \
        -o "cpct${EXT}" \
        src/main.cpp
    echo "Build successful: cpct${EXT}"
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
