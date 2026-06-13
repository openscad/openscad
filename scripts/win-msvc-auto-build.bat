@echo off

rem Set Config Here -------------------------------------------------------------

set "BUILD_GUI=ON"
set "BUILD_TEST=ON"

set "BUILD_NAME=build_msvc"
set "BUILD_TYPE=Release"

set "MODE_MANIFEST=ON"
set "VCPKG_TARGET_TRIPLET=x64-windows"
set "MSVC_TOOLS=Visual Studio 17 2022"
rem set "MSVC_TOOLS=Visual Studio 18 2026"

rem ---------------------------------------------------------------------------


rem check Visual Studio Tool
echo [INFO] Checking for Visual Studio (MSVC Compiler)...

@echo off

if not exist "%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" goto ERROR_NO_VS

for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property displayName`) do set "VS_NAME=%%i"

if "%VS_NAME%"=="" goto ERROR_NO_VS

echo [SUCCESS] Found %VS_NAME%. Ready to compile.

:ERROR_NO_VS
echo [ERROR] Visual Studio C++ build tools (MSVC) not found!
echo         Please install Visual Studio with "Desktop development with C++" workload.
exit /b 1

echo ---------------------------------------------------------

rem Check git tool
echo [INFO] Checking for Git installation...
where git.exe >nul 2>nul

if %errorlevel% neq 0 (
    echo [ERROR] Git is not found.
    echo         Please install Git for Windows and ensure it is added to Environment Variables.
    exit /b 1
) else (
    for /f "tokens=3" %%a in ('git --version') do set "GIT_VER=%%a"
    echo [SUCCESS] Git found ^(Version %GIT_VER%^). Ready to fetch repositories.
)
echo ---------------------------------------------------------

rem check Cmake Tool
echo [INFO] Checking for CMake installation...
where cmake.exe >nul 2>nul

if %errorlevel% neq 0 (
    echo [ERROR] CMake is not found in your system PATH.
    echo         Please install CMake and ensure it is added to Environment Variables.
    exit /b 1
) else (
    for /f "tokens=3" %%a in ('cmake --version ^| findstr /i "version"') do set "CMAKE_VER=%%a"
    echo [SUCCESS] CMake found ^(Version %CMAKE_VER%^). Ready to process.
)
echo ---------------------------------------------------------


rem  Set enveronments ---------------------------------------------------------
echo [INFO] Assume git install on "C:\Program Files\Git\bin\"

where bash.exe >nul 2>nul
if %errorlevel% neq 0 (
    if exist "C:\Program Files\Git\bin\" (
        set "PATH=%PATH%;C:\Program Files\Git\bin"
    )
)


rem Find Path ---------------------------------------------------------
for %%i in ("%~dp0..") do set "OPENSCAD_DIR=%%~fi"
for %%i in ("%~dp0..\..") do set "ROOT_DIR=%%~fi"
set "VCPKG_DIR=%ROOT_DIR%\vcpkg"
set "BUILD_DIR=%OPENSCAD_DIR%\%BUILD_NAME%"
set "WINFLEXBISON_DIR=%ROOT_DIR%\winflexbison3"

echo Current Script Folder : %~dp0
echo Openscad Root Folder  : %OPENSCAD_DIR%
echo VCPKG Folder Location : %VCPKG_DIR%
echo Build Folder Location : %BUILD_DIR%
echo Manifest Mode         : %MODE_MANIFEST%
echo ---------------------------------------------------------

rem  Install openscad submodules
rem  ---------------------------------------------------------
echo [INFO] Updating git submodules in openscad...
cd /d "%OPENSCAD_DIR%"
git submodule update --init --recursive

if %errorlevel% neq 0 (
    echo [ERROR] Git submodule update failed.
    exit /b 1
)

echo ---------------------------------------------------------

rem  Install VCPKG
rem  ---------------------------------------------------------
if exist "%VCPKG_DIR%\" (
    echo [INFO] Found vcpkg directory. Skipping.
) else (
    echo [WARN] vcpkg not found. Cloning repository...
    
    cd /d "%ROOT_DIR%"
    git clone https://github.com/microsoft/vcpkg.git
    
    if %errorlevel% equ 0 (
        echo [SUCCESS] vcpkg cloned successfully.
        
        echo [INFO] Bootstrapping vcpkg...
        cd /d "%VCPKG_DIR%"
        call .\bootstrap-vcpkg.bat
        
        if %errorlevel% equ 0 (
            echo [SUCCESS] vcpkg is ready to use.
        ) else (
            echo [ERROR] vcpkg bootstrap failed.
            exit /b 1
        )
    ) else (
        echo [ERROR] git clone failed.
        exit /b 1
    )
)

echo ---------------------------------------------------------

rem WebFlexBison Tools
echo [INFO] Checking for existing win_flex and win_bison in system...

where win_flex.exe >nul 2>nul
set "SYSTEM_CHECK_BISON_ELV=%errorlevel%"

if %SYSTEM_CHECK_BISON_ELV% equ 0 (
    echo [SUCCESS] winflexbison found in system PATH. Skipping auto-install.
) else (
    rem Download if not exists
    if not exist "%WINFLEXBISON_DIR%\win_flex.exe" (
        echo [WARN] winflexbison not found anywhere. Triggering auto-install...
        mkdir "%WINFLEXBISON_DIR%" 2>nul    
        curl -L -o "%ROOT_DIR%\win_flex_bison.zip" "https://github.com/lexxmark/winflexbison/releases/download/v2.5.25/win_flex_bison-2.5.25.zip"
        tar -xf "%ROOT_DIR%\win_flex_bison.zip" -C "%WINFLEXBISON_DIR%"
        del "%ROOT_DIR%\win_flex_bison.zip" 2>nul
    ) else (
        echo [SUCCESS] winflexbison found in managed folder.
    )
)

rem set path if necessary
if %SYSTEM_CHECK_BISON_ELV% neq 0 (
    if exist "%WINFLEXBISON_DIR%\win_flex.exe" (
        set "PATH=%PATH%;%WINFLEXBISON_DIR%"
    )
)

rem pause

rem check again
where win_flex.exe >nul 2>nul

if %errorlevel% equ 0 (
    echo [SUCCESS] winflexbison verification passed! Ready to build.
) else (
    echo [ERROR] No winflexbison found.
    exit /b 1
)

echo ---------------------------------------------------------

rem  Prepare build-dir
rem  ---------------------------------------------------------
if exist "%BUILD_DIR%\" (
    echo [WARN] Old build directory found. Removing: %BUILD_DIR%
    rmdir /s /q "%BUILD_DIR%"
)

echo [INFO] Creating build directory: %BUILD_DIR%
mkdir "%BUILD_DIR%"


rem  CMake Configuration
rem  ---------------------------------------------------------

if /i "%BUILD_GUI%"=="ON" (
    set "EN_HEADLESS=OFF"
    set "EN_QT=ON"
) else (
    set "EN_HEADLESS=ON"
    set "EN_QT=OFF"
)

echo [INFO] Running CMake configuration...

cmake -B %BUILD_DIR% -G %MSVC_TOOLS% -A x64 -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
-DVCPKG_TARGET_TRIPLET=%VCPKG_TARGET_TRIPLET% ^
-DHEADLESS=%EN_HEADLESS% -DUSE_QT6=%EN_QT% -DENABLE_TESTS=%BUILD_TEST% ^
-DENABLE_MANIFOLD=ON ^
-DENABLE_CAIRO=ON ^
-DUSE_BUILTIN_OPENCSG=ON -DUSE_BUILTIN_MANIFOLD=ON -DUSE_BUILTIN_CLIPPER2=ON ^
-DCMAKE_TOOLCHAIN_FILE="%VCPKG_DIR%/scripts/buildsystems/vcpkg.cmake" ^
-DCMAKE_EXE_LINKER_FLAGS="/manifest:no" -DCMAKE_MODULE_LINKER_FLAGS="/manifest:no" -DCMAKE_SHARED_LINKER_FLAGS="/manifest:no" ^
-DVCPKG_MANIFEST_MODE=%MODE_MANIFEST%

if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

echo ---------------------------------------------------------


rem  Get Installed VCPKG PATH as manifest mode
rem  ---------------------------------------------------------
if /i "%MODE_MANIFEST%"=="ON" (
    set "MY_INSTALLED_VCPKG_PATH=%BUILD_DIR%\vcpkg_installed\%VCPKG_TARGET_TRIPLET%"
) else (
    set "MY_INSTALLED_VCPKG_PATH=%VCPKG_DIR%\installed\%VCPKG_TARGET_TRIPLET%"
)

echo ---------------------------------------------------------

rem  Build
rem  ---------------------------------------------------------
cmake --build %BUILD_DIR% --config %BUILD_TYPE%

if %errorlevel% neq 0 (
    echo [ERROR] Build failed.
    exit /b 1
)

echo [INFO] Build Done.


set "TARGET_BUILD_APP_DIR=%BUILD_DIR%\%BUILD_TYPE%"

echo ---------------------------------------------------------

rem  Copy Qt6 Plugins to Result Build Dir
rem  ---------------------------------------------------------
set "QT_PLUGINS_SRC=%MY_INSTALLED_VCPKG_PATH%\Qt6\plugins"

if exist "%QT_PLUGINS_SRC%\" (
    echo [INFO] Copying contents of Qt6 plugins directly into build directory...
    xcopy "%QT_PLUGINS_SRC%" "%TARGET_BUILD_APP_DIR%" /e /i /h /y /d
    
    if %errorlevel% neq 0 (
        echo [ERROR] Failed to copy Qt6 plugins contents.
        exit /b 1
    )
    echo [SUCCESS] Qt6 plugins contents copied successfully.
) else (
    echo [WARN] Qt6 plugins source folder not found: %QT_PLUGINS_SRC%
)

echo ---------------------------------------------------------


rem Copy Qt*.dll ,when testing some .dll missing ,like Qtsvg.dll 
set "VCPKG_BIN_DIR=%MY_INSTALLED_VCPKG_PATH%\bin"

if exist "%VCPKG_BIN_DIR%\" (
    xcopy "%VCPKG_BIN_DIR%\Qt*.dll" "%TARGET_BUILD_APP_DIR%\" /i /h /y /d
    if %errorlevel% neq 0 (
        echo [ERROR] Failed to copy Qt dll dependencies.
        exit /b 1
    )
    echo [SUCCESS] Copied Qt dll dependencies.
) else (
    echo [WARN] VCPKG-bin folder not found: %VCPKG_BIN_DIR%
    exit /b 1
)

echo ---------------------------------------------------------


if exist "%OPENSCAD_DIR%\shaders" (
    echo [INFO] Copying shaders folder into build directory...
    xcopy "%OPENSCAD_DIR%\shaders" "%TARGET_BUILD_APP_DIR%\shaders" /e /i /h /y /d
    if %errorlevel% neq 0 (
        echo [ERROR] Failed to copy shaders folder.
        exit /b 1
    )
    echo [SUCCESS] Shaders deployed successfully.
) else (
    echo [WARN] Shaders source folder not found at: %SHADER_SRC%
)


echo [SUCCESS] All done. Have a nice day with MSVC-Openscad

