#!/bin/bash

# Advanced OpenGL Demo - Build with Docker (macOS Cross-Compilation)
# Note: This requires osxcross to be set up in the Docker image.
# For easier macOS builds, consider using GitHub Actions instead.

set -e

# Get the directory where this script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/../.."

# Export IDs for docker compose user mapping
export USER_ID=$(id -u)
export GROUP_ID=$(id -g)

sudo -E docker compose -f commands/docker/docker-compose.yml run --rm macos

