cmake_minimum_required(VERSION 3.14) # Required for FetchContent
project(bullet_shift)

# Set OpenGL policy to prefer GLVND
cmake_policy(SET CMP0072 NEW)

# C++ standard requirements
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compiler warnings
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(WIN32)
        # Static linking for MinGW
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -static")
    endif()
endif()

include(FetchContent)

# --- Dependencies ---

# 1. Start with GLM (Math)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        0.9.9.8
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(glm)

# 2. Dear ImGui
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG        docking
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(imgui)

# Manually define ImGui library since it doesn't have a CMakeLists.txt
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR})

# 3. Assimp (Model Loading)
# Turn off Assimp tests and tools to speed up build
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE) # Force static linking for dependencies
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG        v5.2.5
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(assimp)

# 4. Miniaudio (Audio)
# Miniaudio is a single header, but we can fetch it or just download it.
# For robustness, we'll fetch the repo.
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG        0.11.21
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
FetchContent_MakeAvailable(miniaudio)
# Create an interface library for miniaudio since it's header-only (mostly)
# Users need to define MINIAUDIO_IMPLEMENTATION in *one* cpp file.
add_library(miniaudio INTERFACE)
target_include_directories(miniaudio INTERFACE ${miniaudio_SOURCE_DIR})

# 5. GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG        3.3.8
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# 6. OpenGL
find_package(OpenGL REQUIRED)

# Include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/include/Core
    ${CMAKE_SOURCE_DIR}/include/Entities
    ${CMAKE_SOURCE_DIR}/include/Renderer
    ${CMAKE_SOURCE_DIR}/include/Systems
    ${CMAKE_SOURCE_DIR}/include/UI
    ${CMAKE_SOURCE_DIR}/external/glad/include
    ${CMAKE_SOURCE_DIR}/external/stb # For stb_image.h
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

# Add GLAD source
add_library(glad STATIC
    ${CMAKE_SOURCE_DIR}/external/glad/src/gl.c
)

# Source files
file(GLOB_RECURSE SOURCES "src/*.cpp") # Easier for adding new files

# Add ImGui backend sources
set(IMGUI_BACKENDS
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

# Create executable
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_BACKENDS})

# Set output directory to bullet_shift subdirectory
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bullet_shift"
    OUTPUT_NAME "bullet_shift"
)

# Platform-specific executable settings
if(WIN32)
    # Windows: Add .exe extension
    set_target_properties(${PROJECT_NAME} PROPERTIES
        SUFFIX ".exe"
    )
elseif(APPLE)
    # macOS: Create app bundle
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/cmake/Info.plist"
    )
    # Create Info.plist if it doesn't exist
    if(NOT EXISTS "${CMAKE_SOURCE_DIR}/cmake/Info.plist")
        file(WRITE "${CMAKE_BINARY_DIR}/Info.plist"
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
            "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
            "<plist version=\"1.0\">\n"
            "<dict>\n"
            "    <key>CFBundleDevelopmentRegion</key>\n"
            "    <string>en</string>\n"
            "    <key>CFBundleExecutable</key>\n"
            "    <string>${PROJECT_NAME}</string>\n"
            "    <key>CFBundleIdentifier</key>\n"
            "    <string>com.opengldemo.${PROJECT_NAME}</string>\n"
            "    <key>CFBundleInfoDictionaryVersion</key>\n"
            "    <string>6.0</string>\n"
            "    <key>CFBundleName</key>\n"
            "    <string>${PROJECT_NAME}</string>\n"
            "    <key>CFBundlePackageType</key>\n"
            "    <string>APPL</string>\n"
            "    <key>CFBundleShortVersionString</key>\n"
            "    <string>1.0</string>\n"
            "    <key>CFBundleVersion</key>\n"
            "    <string>1</string>\n"
            "</dict>\n"
            "</plist>\n"
        )
        set_target_properties(${PROJECT_NAME} PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_BINARY_DIR}/Info.plist"
        )
    endif()
endif()

# Link libraries
target_link_libraries(${PROJECT_NAME} 
    PRIVATE
    OpenGL::GL 
    glfw
    glad
    glm::glm
    imgui
    assimp
    miniaudio
    ${CMAKE_DL_LIBS}
)

# Copy shaders, assets, and config files to the distribution directory
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_BINARY_DIR}/bullet_shift)
# If assets folder exists, copy it too
if(EXISTS "${CMAKE_SOURCE_DIR}/assets")
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${CMAKE_BINARY_DIR}/bullet_shift
        PATTERN "*.blend" EXCLUDE 
        PATTERN "*.blend1" EXCLUDE
        PATTERN "*.blend2" EXCLUDE
        PATTERN "*.blend3" EXCLUDE
        PATTERN "*.blend4" EXCLUDE
        PATTERN "*.blend5" EXCLUDE
        PATTERN "*.blend6" EXCLUDE
        PATTERN "*.blend7" EXCLUDE
        PATTERN "*.blend8" EXCLUDE
        PATTERN "*.blend9" EXCLUDE
    )
endif()

# Copy configuration files
if(EXISTS "${CMAKE_SOURCE_DIR}/config/settings.ini")
    file(COPY "${CMAKE_SOURCE_DIR}/config/settings.ini" DESTINATION "${CMAKE_BINARY_DIR}/bullet_shift")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/config/imgui.ini")
    file(COPY "${CMAKE_SOURCE_DIR}/config/imgui.ini" DESTINATION "${CMAKE_BINARY_DIR}/bullet_shift")
endif()
if(EXISTS "${CMAKE_SOURCE_DIR}/config/renderer.txt")
    file(COPY "${CMAKE_SOURCE_DIR}/config/renderer.txt" DESTINATION "${CMAKE_BINARY_DIR}/bullet_shift")
endif()

# Create a portable run script for the distribution
file(WRITE "${CMAKE_BINARY_DIR}/bullet_shift/run.sh"
    "#!/bin/bash\n"
    "cd \"$(dirname \"$0\")\"\n"
    "# Force Native Wayland if possible, but fallback is handled by GLFW\n"
    "export GLFW_PLATFORM=wayland\n"
    "./bullet_shift \"$@\"\n"
)
execute_process(COMMAND chmod +x "${CMAKE_BINARY_DIR}/bullet_shift/run.sh")

# Create a helper script for copying assets with exclusion during build
set(COPY_ASSETS_SCRIPT "${CMAKE_BINARY_DIR}/copy_assets.cmake")
file(WRITE "${COPY_ASSETS_SCRIPT}"
    "file(COPY \"${CMAKE_SOURCE_DIR}/assets\" DESTINATION \"\${DEST_DIR}\" \n"
    "    PATTERN \"*.blend\" EXCLUDE \n"
    "    PATTERN \"*.blend1\" EXCLUDE \n"
    "    PATTERN \"*.blend2\" EXCLUDE \n"
    "    PATTERN \"*.blend3\" EXCLUDE \n"
    "    PATTERN \"*.blend4\" EXCLUDE \n"
    "    PATTERN \"*.blend5\" EXCLUDE \n"
    "    PATTERN \"*.blend6\" EXCLUDE \n"
    "    PATTERN \"*.blend7\" EXCLUDE \n"
    "    PATTERN \"*.blend8\" EXCLUDE \n"
    "    PATTERN \"*.blend9\" EXCLUDE)\n"
    "file(COPY \"${CMAKE_SOURCE_DIR}/shaders\" DESTINATION \"\${DEST_DIR}\")\n"
    "if(EXISTS \"${CMAKE_SOURCE_DIR}/config/settings.ini\")\n"
    "    file(COPY \"${CMAKE_SOURCE_DIR}/config/settings.ini\" DESTINATION \"\${DEST_DIR}\")\n"
    "endif()\n"
    "if(EXISTS \"${CMAKE_SOURCE_DIR}/config/imgui.ini\")\n"
    "    file(COPY \"${CMAKE_SOURCE_DIR}/config/imgui.ini\" DESTINATION \"\${DEST_DIR}\")\n"
    "endif()\n"
    "if(EXISTS \"${CMAKE_SOURCE_DIR}/config/renderer.txt\")\n"
    "    file(COPY \"${CMAKE_SOURCE_DIR}/config/renderer.txt\" DESTINATION \"\${DEST_DIR}\")\n"
    "endif()\n"
)

# Robustly copy assets to the executable directory post-build
# This ensures that for multi-config generators (VS, Xcode), assets are next to the .exe
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -DDEST_DIR=$<TARGET_FILE_DIR:${PROJECT_NAME}> -P "${COPY_ASSETS_SCRIPT}"
    COMMENT "Copying assets and shaders to output directory..."
)
