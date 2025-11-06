@echo off
echo ========================================
echo  Drones Neural Network - Build and Run
echo ========================================
echo.

REM Check if build directory exists
if not exist "build" (
    echo Creating build directory...
    mkdir build
)

cd build

REM Run CMake if needed
if not exist "Makefile" (
    echo Running CMake...
    cmake -G "MinGW Makefiles" ..
    if errorlevel 1 (
        echo CMake failed! Make sure CMake and MinGW are installed.
        pause
        exit /b 1
    )
)

REM Build the project
echo.
echo Building project...
mingw32-make
if errorlevel 1 (
    echo Build failed!
    pause
    exit /b 1
)

REM Run the program
echo.
echo Starting simulation...
echo.
nndrons.exe
if errorlevel 1 (
    echo Program execution failed!
    pause
    exit /b 1
)

pause
