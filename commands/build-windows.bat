@echo off
echo ========================================
echo Advanced OpenGL Demo - Build (Windows)
echo ========================================

rem Ensure CMake is in the PATH (Winget install location)
set "PATH=%PATH%;C:\Program Files\CMake\bin"

rem Check if we are running in a VS Developer Command Prompt
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo Setting up Visual Studio environment...
    if exist "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" (
        call "C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\Tools\VsDevCmd.bat" -arch=x64 -host_arch=x64 -vcvars_ver=14.44
    ) else (
        echo Error: Could not find Visual Studio 2022 Community installation.
        echo Please ensure you have Visual Studio 2022 installed with C++ workload.
        pause
        exit /b 1
    )
)

rem Switch to project root directory
pushd %~dp0\..

if not exist build\windows mkdir build\windows
cd build\windows

echo Checking for compiler...
where cl
cl

echo Configuring with CMake...
cmake ..\.. -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5
if %errorlevel% neq 0 (
    echo CMake configuration failed!
    popd
    pause
    exit /b %errorlevel%
)

echo Building...
cmake --build . --config Release
if %errorlevel% neq 0 (
    echo Build failed!
    popd
    pause
    exit /b %errorlevel%
)

echo ========================================
echo Build completed successfully!
echo Executable: build\windows\AdvancedOpenGL.exe
echo ========================================
echo.
echo Press any key to exit...
popd
pause >nul
