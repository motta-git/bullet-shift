#!/bin/bash

# Advanced OpenGL Demo - macOS Cross-Compilation Script (Internal, called from Docker)
# Note: This requires osxcross to be properly set up with macOS SDK.
# See: https://github.com/tpoechtrager/osxcross

set -e

echo "================================"
echo "Advanced OpenGL Demo - macOS Cross-Compilation"
echo "================================"
echo ""

# Check if osxcross is available
OSXCROSS_PATH="/opt/osxcross"
if [ ! -d "$OSXCROSS_PATH" ]; then
    echo "Warning: osxcross not found at $OSXCROSS_PATH"
    echo "macOS cross-compilation requires osxcross to be set up."
    echo "For easier macOS builds, consider using GitHub Actions instead."
    echo ""
    echo "To set up osxcross:"
    echo "  1. Obtain macOS SDK from Xcode"
    echo "  2. Follow instructions at: https://github.com/tpoechtrager/osxcross"
    exit 1
fi

# Set up osxcross environment
export PATH="$OSXCROSS_PATH/bin:$PATH"
export CC=o64-clang
export CXX=o64-clang++

# Verify toolchain is available
if ! command -v $CC &> /dev/null; then
    echo "Error: osxcross toolchain not found. Please ensure osxcross is installed."
    exit 1
fi

if ! command -v cmake &> /dev/null; then
    echo "Error: cmake not found."
    exit 1
fi

echo "Using osxcross toolchain:"
echo "  CC:  $CC"
echo "  CXX: $CXX"
echo ""

# Create build directory
if [ "$1" == "clean" ]; then
    echo "Cleaning build directory..."
    rm -rf build/macos
fi

if [ ! -d "build/macos" ]; then
    echo "Creating build directory..."
    mkdir -p build/macos
fi
if [ -d "build/macos" ] && [ -f "build/macos/CMakeCache.txt" ]; then
    echo "Existing build artifacts found. Performing thorough cleanup to prevent path mismatches..."
    find build/macos -name "CMakeCache.txt" -delete
    find build/macos -name "CMakeFiles" -type d -exec rm -rf {} +
    rm -rf build/macos/_deps
fi

cd build/macos

# Configure with CMake for macOS cross-compilation
echo "Configuring with CMake for macOS..."
cmake ../.. \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_SYSTEM_NAME=Darwin \
    -DCMAKE_SYSTEM_PROCESSOR=x86_64 \
    -DCMAKE_C_COMPILER=$CC \
    -DCMAKE_CXX_COMPILER=$CXX \
    -DCMAKE_OSX_SYSROOT=$OSXCROSS_PATH/SDK/MacOSX.sdk \
    -DCMAKE_FIND_ROOT_PATH=$OSXCROSS_PATH/SDK/MacOSX.sdk \
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
echo "Executable: ./build/macos/bullet_shift/bullet_shift"
echo ""

