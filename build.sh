#!/usr/bin/env bash
# C% Build Script
# Usage:
#   bash build.sh              # Build all
#   bash build.sh transpiler   # Build transpiler only
#   bash build.sh jit          # Build Cling JIT REPL only

set -e

CXX="/mingw64/bin/g++"
CXXFLAGS="-std=c++20 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare"
CORE_SRC="cpct-core/src/lexer.cpp cpct-core/src/parser.cpp"
BIGINT_SRC="cpct-cpp-lib/cpct/data/type/bigint.cpp"

build_transpiler() {
    echo "Building C% Transpiler..."
    $CXX $CXXFLAGS \
        -I cpct-core/src \
        -o cpct-translate.exe \
        cpct-transpiler/src/main.cpp \
        $CORE_SRC \
        $BIGINT_SRC
    echo "Build successful: cpct-translate.exe"
}

build_jit() {
    echo "Building C% Cling JIT REPL..."
    $CXX $CXXFLAGS \
        -I cpct-core/src \
        -o cpct-cling.exe \
        cpct-jit/src/cling_repl.cpp \
        $CORE_SRC \
        $BIGINT_SRC
    echo "Build successful: cpct-cling.exe"
}

case "${1:-all}" in
    transpiler) build_transpiler ;;
    jit)        build_jit ;;
    all)
        build_transpiler
        build_jit
        ;;
    *)
        echo "Usage: bash build.sh [transpiler|jit|all]"
        exit 1
        ;;
esac
