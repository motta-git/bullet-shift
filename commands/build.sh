#!/bin/bash

# Advanced OpenGL Demo - Unified Build Script
# This script handles both native and Docker builds for various platforms.

set -e

# --- Configuration & Defaults ---
USE_DOCKER=false
CLEAN=false
PLATFORM=""
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

# --- Helper Functions ---

print_usage() {
    echo "Usage: ./commands/build.sh [platform] [options]"
    echo ""
    echo "Arguments:"
    echo "  platform          Target platform: linux, macos, or windows"
    echo ""
    echo "Options:"
    echo "  --docker          Build inside a Docker container (requires Docker)"
    echo "  --clean           Perform a clean build (removes build directory and cache)"
    echo "  --platform <p>    Explicitly set target platform (alternative to positional)"
    echo "                    (Defaults to host OS for native, linux for Docker)"
    echo "  --help            Show this help message"
    echo ""
    echo "Examples:"
    echo "  ./commands/build.sh                  # Native build for host OS"
    echo "  ./commands/build.sh --docker         # Linux build in Docker"
    echo "  ./commands/build.sh --docker --platform windows  # Cross-compile for Windows in Docker"
}

detect_os() {
    case "$(uname -s)" in
        Linux*)  PLATFORM="linux";;
        Darwin*) PLATFORM="macos";;
        *)       echo "Error: Unsupported host OS for native build. Use --docker --platform linux|windows|macos"; exit 1;;
    esac
}

# --- Argument Parsing ---

# Check for positional platform parameter first
if [[ "$1" =~ ^(linux|macos|windows)$ ]]; then
    PLATFORM="$1"
    shift
fi

while [[ $# -gt 0 ]]; do
    case $1 in
        --docker)   USE_DOCKER=true; shift ;;
        --clean)    CLEAN=true; shift ;;
        --platform) PLATFORM="$2"; shift 2 ;;
        --help)     print_usage; exit 0 ;;
        linux|macos|windows)
            if [ -z "$PLATFORM" ]; then
                PLATFORM="$1"
                shift
            else
                echo "Error: Platform already specified as $PLATFORM"
                exit 1
            fi
            ;;
        *)          echo "Unknown option: $1"; print_usage; exit 1 ;;
    esac
done

# Set default platform if not specified
if [ -z "$PLATFORM" ]; then
    if [ "$USE_DOCKER" = true ]; then
        PLATFORM="linux"
    else
        detect_os
    fi
fi

# --- Execution ---

cd "$PROJECT_ROOT"

if [ "$USE_DOCKER" = true ]; then
    echo "Building for $PLATFORM using Docker..."
    
    # Export IDs for docker compose user mapping
    export USER_ID=$(id -u)
    export GROUP_ID=$(id -g)
    
    case "$PLATFORM" in
        linux)
            sudo -E docker compose -f commands/docker/docker-compose.yml run --rm linux
            ;;
        windows)
            sudo -E docker compose -f commands/docker/docker-compose.yml run --rm windows
            ;;
        macos)
            sudo -E docker compose -f commands/docker/docker-compose.yml run --rm macos
            ;;
        *)
            echo "Error: Unsupported Docker platform: $PLATFORM"
            exit 1
            ;;
    esac
else
    # Native / Container Internal Build Logic
    echo "Building for $PLATFORM natively..."
    
    # Detect if we are inside a Docker container
    INSIDE_CONTAINER=false
    if [ -f /.dockerenv ]; then
        INSIDE_CONTAINER=true
    fi

    # Use separate build directories for Host vs Container to avoid CMake cache conflicts
    if [ "$INSIDE_CONTAINER" = true ]; then
        BUILD_DIR="build/${PLATFORM}-docker"
        echo "✓ Container environment detected (Using dynamic build path: $BUILD_DIR)"
    else
        BUILD_DIR="build/$PLATFORM"
    fi
    
    if [ "$CLEAN" = true ]; then
        echo "Performing full clean..."
        rm -rf "$BUILD_DIR"
    fi
    
    # Specific logic for each native platform
    if [ "$PLATFORM" = "linux" ]; then
        # Check dependencies
        if ! command -v cmake &> /dev/null; then echo "Error: cmake not found."; exit 1; fi
        if ! command -v g++ &> /dev/null; then echo "Error: g++ not found."; exit 1; fi
        
        # Preserve Assimp if possible, but handle Cache reset
        if [ -f "$BUILD_DIR/CMakeCache.txt" ] && [ "$CLEAN" = false ]; then
            echo "Updating build configuration..."
            rm -f "$BUILD_DIR/CMakeCache.txt"
        fi
        
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        
        cmake ../.. -DCMAKE_BUILD_TYPE=Release
        make -j$(nproc)
        
        echo ""
        echo "Build completed! Executable: ./$BUILD_DIR/bullet_shift/bullet_shift"
        
    elif [ "$PLATFORM" = "macos" ]; then
        # Check dependencies
        if ! command -v cmake &> /dev/null; then echo "Error: cmake not found."; exit 1; fi
        
        if [ -f "$BUILD_DIR/CMakeCache.txt" ] && [ "$CLEAN" = false ]; then
            echo "Updating build configuration..."
            rm -f "$BUILD_DIR/CMakeCache.txt"
        fi
        
        mkdir -p "$BUILD_DIR"
        cd "$BUILD_DIR"
        
        cmake ../.. -DCMAKE_BUILD_TYPE=Release
        JOBS=$(sysctl -n hw.ncpu || echo 2)
        make -j$JOBS
        
        echo ""
        echo "Build completed! Executable: ./$BUILD_DIR/bullet_shift/bullet_shift"
        
    elif [ "$PLATFORM" = "windows" ]; then
        echo "Error: Native Windows build requires 'commands/build-windows.bat' or use '--docker --platform windows'"
        exit 1
    else
        echo "Error: Unsupported platform for native build: $PLATFORM"
        exit 1
    fi
fi
