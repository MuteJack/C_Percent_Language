#!/usr/bin/env bash
# C% Build Script (Linux / macOS)
# For Windows, use build.bat (MSVC)
#
# Usage:
#   bash build.sh              # Build all (checks cling dependency)
#   bash build.sh translator   # Build cpct-translate only
#   bash build.sh compiler     # Build cpct-compile only
#   bash build.sh jit          # Build cpct-jit only (builds cling if needed)
#   bash build.sh cpct         # Build cpct only
#   bash build.sh cling        # Build cling only

set -e

CXX="g++"

GIT_HASH=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_TIME=$(date +%Y%m%d-%H%M%S)
CXXFLAGS="-std=c++20 -O2 -Wall -Wextra -Wno-unused-parameter -Wno-sign-compare -DCPCT_GIT_HASH=\"${GIT_HASH}\" -DCPCT_BUILD_TIME=\"${BUILD_TIME}\""
LOG_FILE="build-${BUILD_TIME}.log"
exec > >(tee -a "$LOG_FILE") 2>&1
CORE_SRC="src/core/lexer.cpp src/core/parser.cpp"
LIB_SRC=$(sed 's|^|src/lib/|' src/lib/sources.txt)
CLING_DIR="tools/cling-build"
NPROC=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

# ============== Cling Build ==============

check_cling() {
    if [[ -f "${CLING_DIR}/bin/cling" ]]; then
        return 0
    fi
    return 1
}

build_cling() {
    # mkdir tools && cd tools
    # git clone https://github.com/root-project/llvm-project.git
    # cd llvm-project
    # git checkout cling-latest
    # cd ..
    # git clone https://github.com/root-project/cling.git
    # cd cling
    # git checkout tags/v1.3
    # cd ..
    # mkdir cling-build && cd cling-build
    # cmake -DLLVM_EXTERNAL_PROJECTS=cling -DLLVM_EXTERNAL_CLING_SOURCE_DIR=../cling/ -DLLVM_ENABLE_PROJECTS="clang" -DLLVM_TARGETS_TO_BUILD="host;NVPTX" -DCMAKE_BUILD_TYPE=Release ../llvm-project/llvm
    # cmake --build . --target clang cling
    # cd ..

    # Check if cling is already built
    if check_cling; then
        echo "Cling already built at ${CLING_DIR}/bin/cling"
        return 0
    fi

    # Build cling from source (LLVM 20)
    echo "========================================="
    echo "Building Cling v1.3 (LLVM 20) from source"
    echo "This will take 1-3 hours..."
    echo "========================================="

    # Check dependencies
    for cmd in git cmake python3; do
        if ! command -v $cmd &>/dev/null; then
            echo "Error: $cmd is required but not installed."
            exit 1
        fi
    done

    mkdir -p tools/.src

    # Clone LLVM + Clang
    if [[ ! -d "tools/.src/llvm-project" ]]; then
        echo "Cloning LLVM (cling-latest branch)..."
        git clone --depth=1 -b cling-latest \
            https://github.com/root-project/llvm-project.git tools/.src/llvm-project
    fi

    # Clone Cling
    if [[ ! -d "tools/.src/cling" ]]; then
        echo "Cloning Cling v1.3..."
        git clone --depth=1 -b v1.3 \
            https://github.com/root-project/cling.git tools/.src/cling
    fi

    # Configure & Build
    mkdir -p tools/cling-build
    cd tools/cling-build

    echo "Configuring CMake..."
    cmake \
        -DLLVM_EXTERNAL_PROJECTS=cling \
        -DLLVM_EXTERNAL_CLING_SOURCE_DIR=../../tools/.src/cling/ \
        -DLLVM_ENABLE_PROJECTS="clang" \
        -DLLVM_TARGETS_TO_BUILD="host;NVPTX" \
        -DCMAKE_BUILD_TYPE=Release \
        ../.src/llvm-project/llvm

    echo "Building (using ${NPROC} cores)..."
    cmake --build . --target clang cling -j${NPROC}

    cd ../..
    echo "Cling build complete: ${CLING_DIR}/bin/cling"
}

# ============== C% Build ==============

build_cpct() {
    echo "Building cpct..."
    $CXX $CXXFLAGS \
        -I src/core \
        -pthread \
        -o "cpct" \
        src/main.cpp \
        $CORE_SRC \
        $LIB_SRC
    echo "Build successful: cpct"
}

install_lib() {
    if [[ ! -d "${CLING_DIR}" ]]; then
        echo "Skipping lib install: ${CLING_DIR} not found."
        return 0
    fi
    echo "Installing cpct library to ${CLING_DIR}/include/..."
    rm -rf "${CLING_DIR}/include/cpct"
    mkdir -p "${CLING_DIR}/include"
    cp -r src/lib/cpct "${CLING_DIR}/include/"
    echo "Done."
}

# ============== Main ==============

case "${1:-all}" in
    cpct)  build_cpct ;;
    cling) build_cling ;;
    all)
        build_cling
        build_cpct
        install_lib
        ;;
    *)
        echo "Usage: bash build.sh [cpct|cling|all]"
        exit 1
        ;;
esac
