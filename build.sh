#!/usr/bin/env bash
# C% Interpreter Build Script
# Usage: C:/msys64/usr/bin/bash.exe -l -c "cd {PROJECT_ROOT} && bash build.sh"
# Note: Windows에서는 WSL bash가 아닌 MSYS2 bash를 사용해야 합니다

set -e

CXX="/mingw64/bin/g++"
CXXFLAGS="-std=c++17 -O2 -Wall -Wextra"
SRC="cpct-interpreter/src/main.cpp cpct-core/src/lexer.cpp cpct-core/src/parser.cpp cpct-interpreter/src/interpreter.cpp cpct-interpreter/src/bigint.cpp"
OUT="cpct.exe"

echo "Building C% Interpreter..."
$CXX $CXXFLAGS $SRC -Icpct-core/src -Icpct-interpreter/src -o $OUT
echo "Build successful: $OUT"
