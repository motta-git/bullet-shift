# Build Commands

This folder contains scripts to build the project.

## The Single Entry Point

Use the main `build.sh` script regardless of your platform or if you want to use Docker.

### For Linux/macOS users:

```bash
# Build natively on your host OS (auto-detects Linux/macOS)
./commands/build.sh

# Build specifically for Linux (useful if you want to force it)
./commands/build.sh linux

# Build inside Docker (Linux by default)
./commands/build.sh linux --docker

# Cross-compile for Windows using Docker
./commands/build.sh windows --docker

# Full clean build
./commands/build.sh --clean
```

### For Windows users:

*   **Native build**: Run `build-windows.bat` (Requires Visual Studio/MSVC).
*   **Docker build**: If you have Git Bash or WSL, you can use `./commands/build.sh --docker`.

---

## Infrastructure (Under the hood)

- `docker/`: Contains the `Dockerfile`, `docker-compose.yml`, and internal cross-compilation entrypoints.
- `build-windows.bat`: Specialized script for native Windows environments.

## System Requirements (Native Builds)

The project automatically fetches most dependencies via CMake's `FetchContent`. However, for native builds, you need:

- **CMake 3.14+**
- **C++17 compatible compiler**
- **OpenGL Development Headers**:
    - Linux: `libgl1-mesa-dev`, `libx11-dev`, `libxi-dev`, `libxcursor-dev`, `libxinerama-dev`, `libxrandr-dev`.
    - macOS: Pre-installed with Xcode.
