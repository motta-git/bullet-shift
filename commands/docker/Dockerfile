FROM ubuntu:22.04

# Avoid interactive prompts
ENV DEBIAN_FRONTEND=noninteractive

# Install all dependencies in one layer
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    ca-certificates \
    ccache \
    # Linux dependencies
    libglfw3-dev \
    libgl1-mesa-dev \
    libxrandr-dev \
    libxinerama-dev \
    libxcursor-dev \
    libxi-dev \
    xorg-dev \
    libwayland-dev \
    wayland-protocols \
    libxkbcommon-dev \
    extra-cmake-modules \
    # Windows cross-compilation
    mingw-w64 \
    g++-mingw-w64 \
    # macOS cross-compilation dependencies (for osxcross)
    clang \
    libxml2-dev \
    libssl-dev \
    libbz2-dev \
    liblzma-dev \
    zlib1g-dev \
    patch \
    wget \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# The rest is handled by volume mount in docker-compose
CMD ["/bin/bash"]
