#!/bin/bash

# Advanced OpenGL Demo - Windows Cross-Compilation Script (Internal, called from Docker)

set -e

echo "================================"
echo "Advanced OpenGL Demo - Windows Cross-Compilation"
echo "================================"
echo ""

# Set up MinGW-w64 toolchain
export CC=x86_64-w64-mingw32-gcc
export CXX=x86_64-w64-mingw32-g++

# Verify toolchain is available
if ! command -v $CC &> /dev/null; then
    echo "Error: MinGW-w64 toolchain not found. Please ensure mingw-w64 is installed."
    exit 1
fi

if ! command -v cmake &> /dev/null; then
    echo "Error: cmake not found."
    exit 1
fi

echo "Using MinGW-w64 toolchain:"
echo "  CC:  $CC"
echo "  CXX: $CXX"
echo ""

# Create build directory
if [ "$1" == "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf build/windows
fi

if [ ! -d "build/windows" ]; then
    echo "Creating build directory..."
    mkdir -p build/windows
fi
# Clean CMake cache and dependency subbuilds if they exist to avoid path mismatch errors between host and container
if [ -d "build/windows" ] && [ -f "build/windows/CMakeCache.txt" ]; then
    echo "Existing build artifacts found. Performing thorough cleanup to prevent path mismatches..."
    find build/windows -name "CMakeCache.txt" -delete
    find build/windows -name "CMakeFiles" -type d -exec rm -rf {} +
    rm -rf build/windows/_deps
fi
cd build/windows

# Configure with CMake for Windows cross-compilation
echo "Configuring with CMake for Windows..."
cmake ../.. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCMAKE_C_COMPILER=$CC \
    -DCMAKE_CXX_COMPILER=$CXX \
    -DCMAKE_FIND_ROOT_PATH=/usr/x86_64-w64-mingw32 \
    -DCMAKE_FIND_ROOT_PATH_MODE_PROGRAM=NEVER \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY

# Build
echo ""
echo "Building..."
make -j$(nproc)

echo ""
echo "================================"
echo "Build completed successfully!"
echo "================================"
echo ""
echo "Executable: ./build/windows/bullet_shift/bullet_shift.exe"
echo ""

