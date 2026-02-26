---
name: build-run
description: Build, run, and test this OpenGL project inside the provided Docker Compose stack so Copilot always uses sudo docker compose and the canonical helper scripts.
---

# Skill Instructions

## When to use
- Any time Copilot needs to build, run, or test the project
- Whenever a command would normally require host toolchains (GLFW, CMake, compilers) that only exist inside the container image
- When referencing the correct command flags for `sudo docker compose`

## Core rules
1. **Never install host dependencies.** All compilers, SDKs, and runtime libs live inside the Docker image defined in `commands/docker/Dockerfile`.
2. **Always prepend `sudo docker compose`** (not `docker-compose`). Password entry is acceptable.
3. **Use the repo root as the working directory** before invoking any helper scripts.
4. **Prefer the Linux builder service** unless a platform-specific path is explicitly required.

## Build helper
- Command: `sudo docker compose -f commands/docker/docker-compose.yml run --rm linux`
- Behavior: launches the `linux` service (inherits from `builder`) and executes `commands/build-linux.sh`, which configures and builds via CMake, copies assets, and emits `./build/linux/AdvancedOpenGL`.
- Re-run behavior: container exits after the build; rerun the same command for incremental builds.

## Run helper
- Command: `sudo docker compose -f commands/docker/docker-compose.yml run --rm linux /bin/bash -lc "./commands/build-linux.sh && ./build/linux/AdvancedOpenGL"`
- Behavior: ensures binaries are up to date, then runs the executable within the container so audio/GL dependencies are satisfied. Adjust the trailing command to execute other binaries as needed.
- Logs: stdout/stderr stream to the host terminal; capture `[Audio]` diagnostics directly from this session.

## Test helper
- If future automated tests are added (for example, `commands/test-linux.sh`), run them through the same service: `sudo docker compose -f commands/docker/docker-compose.yml run --rm linux /bin/bash commands/test-linux.sh`.
- Until such a script exists, manual validation follows the **Run helper** command.

## Troubleshooting tips
- **Permission errors:** ensure the repo root is mounted (volume declared in compose file). The service already maps `${USER_ID}`/`${GROUP_ID}`; pass them via env vars if file ownership looks wrong.
- **Display forwarding:** the `linux` service forwards `DISPLAY`; verify the host X server allows connections (e.g., run `xhost +local:` once on the host if rendering windows fails).
- **Long-running sessions:** if you need an interactive shell, run `sudo docker compose -f commands/docker/docker-compose.yml run --rm linux /bin/bash` and execute helper scripts manually within that shell.
