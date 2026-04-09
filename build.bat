@echo off
REM C% Build Script for Windows (MSVC)
REM Run from "x64 Native Tools Command Prompt for VS 2022"
REM or any terminal after running vcvars64.bat
REM
REM Usage:
REM   build.bat              Build all (cpct, translate, compile, jit)
REM   build.bat cpct         Build cpct.exe only
REM   build.bat translator   Build cpct-translate.exe only
REM   build.bat compiler     Build cpct-compile.exe only
REM   build.bat jit          Build cpct-jit.exe only
REM   build.bat cling        Build cling from source
REM   build.bat clean        Remove build directory

REM Check if cl.exe is available
where cl >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Error: cl.exe not found. Run this from "x64 Native Tools Command Prompt for VS 2022"
    echo Or run: "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
    exit /b 1
)

set TARGET=%1
if "%TARGET%"=="" set TARGET=all

if "%TARGET%"=="clean" (
    echo Cleaning build directory...
    if exist build rd /s /q build
    echo Done.
    exit /b 0
)

if "%TARGET%"=="cling" goto :build_cling

REM === Build C% executables ===
echo Configuring CMake...
cmake -B build -G "Visual Studio 17 2022" -A x64 -Thost=x64

if "%TARGET%"=="all" goto :build_all
if "%TARGET%"=="cpct" (
    echo Building cpct.exe...
    cmake --build build --config Release --target cpct
) else if "%TARGET%"=="translator" (
    echo Building cpct-translate.exe...
    cmake --build build --config Release --target cpct-translate
) else if "%TARGET%"=="compiler" (
    echo Building cpct-compile.exe...
    cmake --build build --config Release --target cpct-compile
) else if "%TARGET%"=="jit" (
    echo Building cpct-jit.exe...
    cmake --build build --config Release --target cpct-jit
) else (
    echo Usage: build.bat [all^|cpct^|translator^|compiler^|jit^|cling^|clean]
    exit /b 1
)

if %ERRORLEVEL% neq 0 (
    echo Build failed.
    exit /b 1
)
echo Build successful.
exit /b 0

REM === Build All (cling → cpct → install lib) ===
:build_all
call :build_cling
if %ERRORLEVEL% neq 0 (
    echo Cling build failed. Skipping cpct build.
    exit /b 1
)
echo Configuring CMake...
cmake -B build -G "Visual Studio 17 2022" -A x64 -Thost=x64
echo Building all targets...
cmake --build build --config Release --target cpct cpct-translate cpct-compile cpct-jit
if %ERRORLEVEL% neq 0 (
    echo Build failed.
    exit /b 1
)
echo Installing cpct library to tools\cling-build\include\...
if exist tools\cling-build\include\cpct rd /s /q tools\cling-build\include\cpct
xcopy /E /I /Y src\lib\cpct tools\cling-build\include\cpct >nul
echo Build successful.
exit /b 0

REM === Build Cling ===
:build_cling
if exist tools\cling-build\bin\cling.exe (
    echo Cling already built at tools\cling-build\bin\cling.exe
    exit /b 0
)

echo =========================================
echo Building Cling v1.3 (LLVM 20) from source
echo This will take 1-3 hours...
echo =========================================

where git >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Error: git is required but not found.
    exit /b 1
)
where python >nul 2>&1
if %ERRORLEVEL% neq 0 (
    echo Error: python is required but not found.
    exit /b 1
)

if not exist tools\.src mkdir tools\.src

if not exist tools\.src\llvm-project (
    echo Cloning LLVM...
    git clone --depth=1 -b cling-latest https://github.com/root-project/llvm-project.git tools\.src\llvm-project
)

if not exist tools\.src\cling (
    echo Cloning Cling v1.3...
    git clone --depth=1 -b v1.3 https://github.com/root-project/cling.git tools\.src\cling
)

if not exist tools\cling-build mkdir tools\cling-build
cd tools\cling-build

echo Configuring CMake...
cmake -G "Visual Studio 17 2022" -A x64 -Thost=x64 ^
    -DLLVM_EXTERNAL_PROJECTS=cling ^
    -DLLVM_EXTERNAL_CLING_SOURCE_DIR=../.src/cling/ ^
    -DLLVM_ENABLE_PROJECTS="clang" ^
    -DLLVM_TARGETS_TO_BUILD="host;NVPTX" ^
    -DCMAKE_BUILD_TYPE=Release ^
    ../.src/llvm-project/llvm

echo Building (this will take a while)...
cmake --build . --config Release --target clang cling

if %ERRORLEVEL% neq 0 (
    echo Cling build failed.
    cd ..\..
    exit /b 1
)

cd ..\..

REM Copy from Release/ to flat structure for cpct-jit to find
if not exist tools\cling-build\bin mkdir tools\cling-build\bin
copy /Y tools\cling-build\Release\bin\cling.exe tools\cling-build\bin\ >nul
if not exist tools\cling-build\lib mkdir tools\cling-build\lib
xcopy /E /I /Y tools\cling-build\Release\lib\clang tools\cling-build\lib\clang >nul

echo Cling build complete.
echo Built to tools\cling-build\
exit /b 0
