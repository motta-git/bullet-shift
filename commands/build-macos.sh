#!/bin/bash

# Advanced OpenGL Demo - Build and Verification Script (macOS)

set -e

# Get the directory where this script is located and change to project root
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."

echo "================================"
echo "Advanced OpenGL Demo - Build (macOS)"
echo "================================"
echo ""

# Check for required dependencies
echo "Checking dependencies..."
if ! command -v cmake &> /dev/null; then
    echo "Error: cmake not found. Please install cmake (brew install cmake)."
    exit 1
fi

if ! command -v clang++ &> /dev/null; then
    # On macOS, g++ is usually a symlink to clang++, but let's check for clang++ or g++
    if ! command -v g++ &> /dev/null; then
        echo "Error: Compiler not found. Please install Xcode Command Line Tools (xcode-select --install)."
        exit 1
    fi
fi

echo "Delegating dependency checks to CMake..."

echo "✓ All dependencies found"
echo ""

# Avoid wiping _deps every time to preserve the long Assimp compilation
if [ "$1" == "clean" ]; then
    echo "Performing full clean including dependencies..."
    rm -rf build/macos
else
    # Only remove main cache to force re-evaluation of project files without nuking Assimp build
    if [ -f "build/macos/CMakeCache.txt" ]; then
        echo "Updating build configuration..."
        rm -f build/macos/CMakeCache.txt
    fi
fi

mkdir -p build/macos
cd build/macos

# Configure with CMake
echo "Configuring with CMake..."
CMAKE_OPTS="-DCMAKE_BUILD_TYPE=Release"

cmake ../.. $CMAKE_OPTS

# Build
echo ""
echo "Building..."
# Use sysctl on macOS to get CPU count
JOBS=$(sysctl -n hw.ncpu || echo 2)
make -j$JOBS

echo ""

echo ""
echo "To run:"
echo "  cd build/macos/bullet_shift"
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
echo "Executable: ./build/macos/bullet_shift/bullet_shift"
