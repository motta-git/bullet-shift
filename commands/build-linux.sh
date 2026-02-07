#!/bin/bash

# Advanced OpenGL Demo - Build and Verification Script (Linux)

set -e

# Get the directory where this script is located and change to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "================================"
echo "Advanced OpenGL Demo - Build (Linux)"
echo "================================"
echo ""

# Check for required dependencies
echo "Checking dependencies..."
if ! command -v cmake &> /dev/null; then
    echo "Error: cmake not found. Please install cmake."
    exit 1
fi

if ! command -v g++ &> /dev/null; then
    echo "Error: g++ not found. Please install build-essential."
    exit 1
fi

# GLFW is still a system dependency in our Dockerfile, so we can keep this check check or remove it.
# CMake will check it anyway. Let's just trust CMake.
echo "Delegating dependency checks to CMake..."

echo "✓ All dependencies found"
echo ""

# Avoid wiping _deps every time to preserve the long Assimp compilation
# We only delete the cache and build files if 'clean' is passed manually
if [ "$1" == "clean" ]; then
    echo "Performing full clean including dependencies..."
    rm -rf build/linux
else
    # Only remove main cache to force re-evaluation of project files without nuking Assimp build
    if [ -f "build/linux/CMakeCache.txt" ]; then
        echo "Updating build configuration..."
        rm -f build/linux/CMakeCache.txt
    fi
fi

mkdir -p build/linux
cd build/linux

# Configure with CMake (CCache support if available)
echo "Configuring with CMake..."
CMAKE_OPTS="-DCMAKE_BUILD_TYPE=Release"
# if command -v ccache &> /dev/null; then
#     echo "✓ ccache found, enabling for faster compilation"
#     CMAKE_OPTS="$CMAKE_OPTS -DCMAKE_CXX_COMPILER_LAUNCHER=ccache -DCMAKE_C_COMPILER_LAUNCHER=ccache"
# fi

cmake ../.. $CMAKE_OPTS

# Build
echo ""
echo "Building..."
make -j$(nproc)

echo ""

echo ""

echo ""
echo "To run:"
echo "  cd build/linux/bullet_shift"
echo "  ./bullet_shift"
echo ""
echo "Controls:"
echo "  W/A/S/D - Move camera"
echo "  Mouse   - Look around"
echo "  Space   - Move up"
echo "  Shift   - Move down"
echo "  ESC     - Exit"
echo ""
echo "================================"
echo "Build completed successfully!"
echo "================================"
echo ""
echo "Executable: ./build/linux/bullet_shift/bullet_shift"