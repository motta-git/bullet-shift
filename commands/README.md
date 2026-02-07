# Build Commands

This folder contains scripts to build the project on various platforms.

## System Requirements and Dependencies

The project automatically fetches most dependencies via CMake's `FetchContent`. However, you need the following installed on your system if you are building **natively**:

- CMake 3.14+ (Required for dependency fetching)
- C++17 compatible compiler (GCC 9+, Clang 10+, or MSVC 2019+)
- OpenGL Development Headers:
    - Linux: `libgl1-mesa-dev`, `libx11-dev`, `libxi-dev`, `libxcursor-dev`, `libxinerama-dev`, `libxrandr-dev` (depending on your distro).
    - Windows/macOS: Usually included with your IDE or OS SDKs.
- OpenGL 4.6+ compatible drivers and hardware.

The following libraries are used and automatically managed by the build system:
`GLFW 3.3+`, `GLAD`, `GLM`, `Assimp`, `Dear ImGui`, `miniaudio`, and `stb_image`.

## Native Builds
Use these scripts if you have a development environment set up on your machine (CMake, Compiler, etc.).

*   Windows: `build-windows.bat` (Requires CMake and Visual Studio/MSVC)
*   Linux: `build-linux.sh` (Requires CMake, GCC/Clang, Make)
*   macOS: `build-macos.sh` (Requires CMake, Clang/Xcode Tools, Make) -> (I haven't been able to test this sh because i don't own a mac so it might not work)

## Docker / Cross-Compilation
Use these scripts if you want to build using Docker (no local dependencies required except Docker).

*   Linux: `docker/build-linux.sh` (Builds for Linux inside Docker)
*   Windows: `docker/build-windows.sh` (Cross-compiles for Windows inside Docker)
*   macOS: `docker/build-macos.sh` (Cross-compiles for macOS inside Docker - i haven't been able to test this sh because i don't own a mac so it might not work)
