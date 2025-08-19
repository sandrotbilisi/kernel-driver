@echo off
echo === Roblox Offset Dumper Build Script ===
echo.

:: Check if running as administrator
net session >nul 2>&1
if %errorLevel% neq 0 (
    echo ERROR: This script must be run as Administrator
    echo Right-click and select "Run as administrator"
    pause
    exit /b 1
)

:: Set Visual Studio environment
echo Setting up Visual Studio environment...
call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
if %errorLevel% neq 0 (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
    if %errorLevel% neq 0 (
        call "C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat" >nul 2>&1
        if %errorLevel% neq 0 (
            call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul >&1
            if %errorLevel% neq 0 (
                echo ERROR: Visual Studio not found
                echo Please install Visual Studio 2019 or 2022
                pause
                exit /b 1
            )
        )
    )
)

echo Visual Studio environment set up successfully
echo.

:: Create build directories
if not exist "x64build" mkdir x64build
if not exist "x64build\um" mkdir x64build\um

:: Build kernel driver
echo Building kernel driver...
cd centipede-driver
if exist "centipede-driver.sln" (
    msbuild centipede-driver.sln /p:Configuration=Release /p:Platform=x64 /verbosity:minimal
    if %errorLevel% neq 0 (
        echo ERROR: Failed to build kernel driver
        cd ..
        pause
        exit /b 1
    )
) else (
    echo ERROR: Kernel driver solution not found
    cd ..
    pause
    exit /b 1
)
cd ..

:: Build user-mode application
echo Building user-mode application...
cd um
if exist "um.vcxproj" (
    msbuild um.vcxproj /p:Configuration=Release /p:Platform=x64 /verbosity:minimal
    if %errorLevel% neq 0 (
        echo ERROR: Failed to build user-mode application
        cd ..
        pause
        exit /b 1
    )
) else (
    echo ERROR: User-mode project not found
    cd ..
    pause
    exit /b 1
)
cd ..

:: Copy built files to output directory
echo Copying built files...
copy "x64build\centipede-driver\centipede-driver.sys" "x64build\" >nul 2>&1
copy "x64build\centipede-driver\centipede-driver.inf" "x64build\" >nul 2>&1
copy "x64build\um\Release\um.exe" "x64build\" >nul 2>&1

:: Check if files were built successfully
if not exist "x64build\centipede-driver.sys" (
    echo ERROR: Kernel driver not built successfully
    pause
    exit /b 1
)

if not exist "x64build\um.exe" (
    echo ERROR: User-mode application not built successfully
    pause
    exit /b 1
)

echo.
echo === Build completed successfully! ===
echo.
echo Built files:
echo - x64build\centipede-driver.sys (Kernel driver)
echo - x64build\centipede-driver.inf (Driver installation file)
echo - x64build\um.exe (User-mode application)
echo.

:: Ask if user wants to install the driver
echo Do you want to install the kernel driver? (y/n)
set /p install_driver=

if /i "%install_driver%"=="y" (
    echo.
    echo Installing kernel driver...
    
    :: Install driver
    pnputil /add-driver "x64build\centipede-driver.inf" /install
    if %errorLevel% neq 0 (
        echo ERROR: Failed to install driver
        echo Make sure test mode is enabled: bcdedit /set testsigning on
        pause
        exit /b 1
    )
    
    :: Create and start service
    sc create centipede type= kernel binPath= "%cd%\x64build\centipede-driver.sys" >nul 2>&1
    sc start centipede >nul 2>&1
    
    if %errorLevel% neq 0 (
        echo WARNING: Failed to start driver service
        echo You may need to start it manually: sc start centipede
    ) else (
        echo Driver installed and started successfully
    )
    
    echo.
    echo You can now run the user-mode application:
    echo x64build\um.exe
)

echo.
echo Build script completed.
pause
