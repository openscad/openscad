@echo off

rem Set Config Here -------------------------------------------------------------

set "BUILD_GUI=ON"
set "BUILD_TEST=ON"

set "BUILD_NAME=build_msvc"
set "BUILD_TYPE=Release"

set "MODE_MANIFEST=ON"
set "VCPKG_TARGET_TRIPLET=x64-windows"

rem set "CMAKE_VS_GENERATOR=Visual Studio 17 2022"
rem set "CMAKE_VS_GENERATOR=Visual Studio 18 2026"
rem set "CMAKE_VS_GENERATOR=Ninja"
set "CMAKE_VS_GENERATOR="

rem Note -----------------------------------------------------------
rem if CMAKE_VS_GENERATOR is empty ,it will try auto-detct
rem for Ninja ,must have enveronments before this script ,like open this script in x64-msvc-developer-prompt 
rem End Config---------------------------------------------------------------------------

rem -------------------------------------------------------------------------
rem  Initial Path
rem -------------------------------------------------------------------------
for %%i in ("%~dp0..") do set "OPENSCAD_PATH=%%~fi"
for %%i in ("%~dp0..\..") do set "ROOT_PATH=%%~fi"

set "DEPTOOLS=%ROOT_PATH%\deptools"
set "WINFLEXBISON_PATH=%DEPTOOLS%\winflexbison"
set "GIT_LINUX_PATH=%DEPTOOLS%\git-portable"
set "GETTEXT_PATH=%DEPTOOLS%\gettext"

set "BUILD_PATH=%OPENSCAD_PATH%\%BUILD_NAME%"

if not exist "%DEPTOOLS%" mkdir "%DEPTOOLS%" 2>nul

rem -------------------------------------------------------------------------
rem VCPKG JSON FILE
rem -------------------------------------------------------------------------
echo [INFO] Create own vcpkg file

SET "MANIFEST_DIR=%OPENSCAD_PATH%\tmp"
if not exist "%MANIFEST_DIR%" mkdir "%MANIFEST_DIR%"

(
echo {
echo  "name": "openscad",
echo  "version-string": "1.0.0",
echo  "dependencies": [
echo    "boost-regex",
echo    "boost-program-options",
echo    "boost-container",
echo    "boost-assign",
echo    "boost-nowide",
echo    "boost-dll",
echo    "boost-beast",
echo    "eigen3",
echo    "cgal",
echo    "cairo",
echo    "glad",
echo    "harfbuzz",
echo    "fontconfig",
echo    "double-conversion",
echo    "opencsg",
echo    "libxml2",
echo    "libzip",
echo    "glib",
echo    "gperf",
echo    "tbb",
echo    "lib3mf",
echo    "clipper2",
echo    "mimalloc",
echo    "manifold",
echo    "catch2",
echo    "qscintilla",
echo    {
echo      "name": "qtbase",
echo      "features": ["opengl"]
echo    },
echo    "qt5compat",
echo    "qtdeclarative",
echo    "qtmultimedia"
echo  ],
echo  "builtin-baseline": "44819aa2a6c10e56065e2b0330e7d6c89d1d2574"
echo }
) > "%MANIFEST_DIR%\vcpkg.json"

rem -------------------------------------------------------------------------
rem Prepare Build Tools 
rem -------------------------------------------------------------------------
set "has_win_flex=0"
set "has_sed=0"
set "has_msgfmt=0"

where win_flex >nul 2>nul && set "has_win_flex=1"
where sed      >nul 2>nul && set "has_sed=1"
where msgfmt   >nul 2>nul && set "has_msgfmt=1"

rem -------------------------------------------------------------------------
rem  WinFlexBison
rem -------------------------------------------------------------------------
if "%has_win_flex%"=="0" if not exist "%WINFLEXBISON_PATH%\win_flex.exe" (
    echo [WARN] winflexbison not found in deptools. Triggering auto-install...
    mkdir "%WINFLEXBISON_PATH%" 2>nul    
    
    curl -L -o "%ROOT_PATH%\win_flex_bison.zip" "https://github.com/lexxmark/winflexbison/releases/download/v2.5.25/win_flex_bison-2.5.25.zip"
    tar -xf "%ROOT_PATH%\win_flex_bison.zip" -C "%WINFLEXBISON_PATH%"
    del "%ROOT_PATH%\win_flex_bison.zip" 2>nul
    
    if not exist "%WINFLEXBISON_PATH%\win_flex.exe" (
        echo [ERROR] win_flex.exe missing! Download failed or zip corrupted.
        exit /b 1
    )
    
    echo [SUCCESS] winflexbison deployed.
)
rem -------------------------------------------------------------------------
rem  PortableGit
rem -------------------------------------------------------------------------
if "%has_sed%"=="0" if not exist "%GIT_LINUX_PATH%\usr\bin\sed.exe" (
    echo [WARN] PortableGit tools not found in deptools. Triggering auto-install...
    mkdir "%GIT_LINUX_PATH%" 2>nul
    
    echo [INFO] Fetching PortableGit payload from GitHub...
    curl -L -A "Mozilla/5.0" -o "%ROOT_PATH%\git-portable.exe" "https://github.com/git-for-windows/git/releases/download/v2.45.2.windows.1/PortableGit-2.45.2-64-bit.7z.exe"
    
    if not exist "%ROOT_PATH%\git-portable.exe" (
        echo [ERROR] git-portable.exe missing! Download failed or zip corrupted.
        exit /b 1
    )
    
    echo [INFO] Extracting PortableGit into deptools...
    rem --- 
    "%ROOT_PATH%\git-portable.exe" -y -o"%GIT_LINUX_PATH%" >nul
    del "%ROOT_PATH%\git-portable.exe" 2>nul
     
    echo [SUCCESS] PortableGit core utilities deployed successfully.
)
rem -------------------------------------------------------------------------
rem  GetText
rem -------------------------------------------------------------------------

if "%has_msgfmt%"=="0" if not exist "%GETTEXT_PATH%\bin\msgfmt.exe" (
    echo [WARN] msgfmt not found in deptools. Injecting lightweight Gettext...
    mkdir "%GETTEXT_PATH%" 2>nul
    echo [INFO] Downloading Gettext tools...
    curl -L -A "Mozilla/5.0" -o "%ROOT_PATH%\gettext.zip" "https://github.com/mlocati/gettext-iconv-windows/releases/download/v0.21-v1.16/gettext0.21-iconv1.16-shared-64.zip"
    echo [INFO] Extracting Gettext...
    tar -xf "%ROOT_PATH%\gettext.zip" -C "%GETTEXT_PATH%"
    del "%ROOT_PATH%\gettext.zip" 2>nul
    
     if not exist "%GETTEXT_PATH%\bin\msgfmt.exe" (
        echo [ERROR] msgfmt.exe missing! Download failed or zip corrupted.
        exit /b 1
    )
    
    echo [SUCCESS] Gettext utilities deployed successfully.
)


rem -------------------------------------------------------------------------
rem Set Path 
rem -------------------------------------------------------------------------
set "PATH=%WINFLEXBISON_PATH%;%GETTEXT_PATH%\bin;%PATH%;%GIT_LINUX_PATH%\usr\bin"
echo -------------------------------------------------------------------------


rem -------------------------------------------------------------------------
rem Check Basic tools
rem -------------------------------------------------------------------------   

rem check win_flex
where win_flex.exe >nul 2>nul
if %errorlevel% neq 0 (
    echo [ERROR] No win_flex found.
    exit /b 1
)

rem git tool
echo [INFO] Checking for Git installation...
where git.exe >nul 2>nul

if %errorlevel% neq 0 (
    echo [ERROR] Git is not found.
    exit /b 1
) else (
    for /f "tokens=3" %%a in ('git --version') do set "GIT_VER=%%a"
    echo [SUCCESS] Git found ^(Version %GIT_VER%^). Ready to fetch repositories.
)
echo ---------------------------------------------------------

rem  cmake Tool
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


rem check msgfmt
echo [INFO] Checking for msgfmt...
where msgfmt >nul 2>&1

if %errorlevel% neq 0 (
    echo [ERROR] No msgfmt found.
    exit /b 1
)

rem check python 
echo [INFO] Checking for Python environment...

python --version >nul 2>nul

if %errorlevel% neq 0 (
    echo.
    echo [ERROR] No python found!
    echo Don't forget to Disable Windows App Execution on Python
    exit /b 1
)

rem -------------------------------------------------------------------------
rem Check Ninja
rem ------------------------------------------------------------------------- 
rem test if have ninja ,may be build unter Deleloper option mvsc-tools command-prompt
where ninja >nul 2>nul
set "TEST_NINJ_ELV=%errorlevel%"

if "%CMAKE_VS_GENERATOR%" equ "" (
  if %TEST_NINJ_ELV% equ 0 (
      echo [INFO] Ninja is installed ,auto load to Ninja!
      echo.
      ninja --version
      set "CMAKE_VS_GENERATOR=Ninja"
  )
)

rem -------------------------------------------------------------------------
rem Check Visual Studio
rem ------------------------------------------------------------------------- 
rem Find Visual Studio Version if still not set CMAKE_VS_GENERATOR 
echo [INFO] Checking for Visual Studio (MSVC Compiler)...

set "VS_STUDIO_VERSION="

if "%CMAKE_VS_GENERATOR%" equ "" (
    for /f "usebackq delims=. tokens=1" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion`) do set "VS_STUDIO_VERSION=%%i"
)

if "%VS_STUDIO_VERSION%"=="18" (
    echo [INFO] Auto-Detect MSVC Version %VS_STUDIO_VERSION%
    set "CMAKE_VS_GENERATOR=Visual Studio 18 2026"
) else if "%VS_STUDIO_VERSION%"=="17" (
    echo [INFO] Auto-Detect MSVC Version %VS_STUDIO_VERSION%
    set "CMAKE_VS_GENERATOR=Visual Studio 17 2022"
)


rem -------------------------------------------------------------------------
rem Check Setting CMake Generator
rem ------------------------------------------------------------------------- 
if "%CMAKE_VS_GENERATOR%" equ "" (
    echo [ERROR] No build tools found!
    exit /b 1 
)

rem -------------------------------------------------------------------------
rem VCPKG
rem ------------------------------------------------------------------------- 
rem 
rem user already have vcpkg 
if defined VCPKG_ROOT (
    if exist "%VCPKG_ROOT%\scripts\buildsystems\vcpkg.cmake" (
        echo [INFO] Found VCPKG_ROOT env variable.
        set "VCPKG_PATH=%VCPKG_ROOT%"
    )
) else (
  echo [INFO] NO VCPKG_ROOT env variable ,try auto-install
  set "VCPKG_PATH=%ROOT_PATH%\vcpkg"
)

rem --- prevent Defined from Microsoft ,it will bug when building because limit lenght path
echo "%VCPKG_PATH%" | findstr /I /C:"Microsoft Visual Studio" >nul

if %errorlevel% equ 0 (
    echo [WARNING] Override VCPKG Root From Microsoft, preventing errors.
    set "VCPKG_PATH=%ROOT_PATH%\vcpkg"
)

rem  ---------------------------------------------------------
if exist "%VCPKG_PATH%\scripts\buildsystems\vcpkg.cmake" (
    echo [INFO] Found vcpkg. Skipping.
) else (
    echo [WARN] VCKPG Cloning repository...
    
    cd /d "%ROOT_PATH%"
    git clone https://github.com/microsoft/vcpkg.git
    
    echo [INFO] Bootstrapping vcpkg...
    cd /d "%VCPKG_PATH%"
    call .\bootstrap-vcpkg.bat
)

if exist "%VCPKG_PATH%\vcpkg.exe" (
    echo [INFO] Found vcpkg.exe
) else (
    echo [ERROR] No vcpkg.exe found : "%VCPKG_PATH%\vcpkg.exe"
    exit /b 1
)
rem -------------------------------------------------------------------------
rem Summary Info
rem ------------------------------------------------------------------------- 

echo Current Script Folder : %~dp0
echo Openscad Root Location : %OPENSCAD_PATH%
echo DEPTOOLS              : %DEPTOOLS%
echo VCPKG Folder Location : %VCPKG_PATH%
echo Build Folder Location : %BUILD_PATH%
echo Manifest              : %MODE_MANIFEST%
echo CMake Generator       : %CMAKE_VS_GENERATOR% 
echo ---------------------------------------------------------

timeout /t 5 /nobreak >nul


rem -------------------------------------------------------------------------
rem Start Process
rem ------------------------------------------------------------------------- 
echo [INFO] Begin Process
echo ---------------------------------------------------------

echo [INFO] Updating git submodules in openscad...
cd /d "%OPENSCAD_PATH%"
git submodule update --init --recursive

if %errorlevel% neq 0 (
    echo [ERROR] Git submodule update failed.
    exit /b 1
)

echo ---------------------------------------------------------

echo [INFO] Enter into Openscad
cd /d "%OPENSCAD_PATH%"

rem  Prepare build folder
rem  ---------------------------------------------------------
if exist "%BUILD_NAME%\" (
    echo [WARN] Old build directory found. Removing: %BUILD_NAME%
    rmdir /s /q "%BUILD_NAME%"
)

echo [INFO] Creating build directory: %BUILD_NAME%
mkdir "%BUILD_NAME%"


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

cmake -B "%BUILD_NAME%" -G "%CMAKE_VS_GENERATOR%" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% ^
-DVCPKG_TARGET_TRIPLET=%VCPKG_TARGET_TRIPLET% ^
-DHEADLESS=%EN_HEADLESS% -DUSE_QT6=%EN_QT% -DENABLE_TESTS=%BUILD_TEST% ^
-DENABLE_MANIFOLD=ON ^
-DENABLE_CAIRO=ON ^
-DUSE_BUILTIN_OPENCSG=ON -DUSE_BUILTIN_MANIFOLD=OFF -DUSE_BUILTIN_CLIPPER2=OFF ^
-DCMAKE_TOOLCHAIN_FILE="%VCPKG_PATH%/scripts/buildsystems/vcpkg.cmake" ^
-DCMAKE_EXE_LINKER_FLAGS="/manifest:no" -DCMAKE_MODULE_LINKER_FLAGS="/manifest:no" -DCMAKE_SHARED_LINKER_FLAGS="/manifest:no" ^
-DVCPKG_MANIFEST_DIR="%MANIFEST_DIR%" ^
-DVCPKG_MANIFEST_MODE=%MODE_MANIFEST%

rem -DVCPKG_BINARY_SOURCES="clear;default,writable" -DVCPKG_INSTALL_OPTIONS="--clean-after-build"

if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed.
    exit /b 1
)

echo ---------------------------------------------------------

rem  Get Installed VCPKG PATH as manifest mode
rem  ---------------------------------------------------------
if /i "%MODE_MANIFEST%"=="ON" (
    set "MY_INSTALLED_VCPKG_PATH=%BUILD_NAME%\vcpkg_installed\%VCPKG_TARGET_TRIPLET%"
) else (
    set "MY_INSTALLED_VCPKG_PATH=%VCPKG_PATH%\installed\%VCPKG_TARGET_TRIPLET%"
)

echo ---------------------------------------------------------

rem  Build
rem  ---------------------------------------------------------
cmake --build %BUILD_NAME% --config %BUILD_TYPE%

if /i "%CMAKE_VS_GENERATOR%"=="Ninja" (
set "TARGET_BUILD_APP_PATH=%BUILD_NAME%"
) else (
set "TARGET_BUILD_APP_PATH=%BUILD_NAME%\%BUILD_TYPE%"
)

if not exist "%TARGET_BUILD_APP_PATH%\openscad.exe" (
    echo [ERROR] Build failed - openscad.exe not found.
    exit /b 1
) else (
echo [INFO] Build Done.
)

timeout /t 5

echo ---------------------------------------------------------

rem  Copy Qt6 Plugins to Result Build 
rem  ---------------------------------------------------------
set "QT_PLUGINS_SRC=%MY_INSTALLED_VCPKG_PATH%\Qt6\plugins"

if exist "%QT_PLUGINS_SRC%\" (
    echo [INFO] Copying contents of Qt6 plugins directly into build directory...
    xcopy "%QT_PLUGINS_SRC%" "%TARGET_BUILD_APP_PATH%" /e /i /h /y /d
) else (
    echo [WARN] Qt6 plugins source folder not found: %QT_PLUGINS_SRC%
)

echo ---------------------------------------------------------


rem Copy Qt*.dll ,when testing some .dll missing ,like Qtsvg.dll 
set "VCPKG_BIN_PATH=%MY_INSTALLED_VCPKG_PATH%\bin"

if exist "%VCPKG_BIN_PATH%\" (
    xcopy "%VCPKG_BIN_PATH%\Qt*.dll" "%TARGET_BUILD_APP_PATH%\" /i /h /y /d
) else (
    echo [WARN] VCPKG-bin folder not found: %VCPKG_BIN_PATH%
)

echo ---------------------------------------------------------


if exist "%OPENSCAD_PATH%\shaders" (
    echo [INFO] Copying shaders folder into build directory...
    xcopy "%OPENSCAD_PATH%\shaders" "%TARGET_BUILD_APP_PATH%\shaders" /e /i /h /y /d
) else (
    echo [WARN] Shaders source folder not found at: %SHADER_SRC%
)

if exist "%OPENSCAD_PATH%\fonts" (
    echo [INFO] Copying fonts folder into build directory...
    xcopy "%OPENSCAD_PATH%\fonts" "%TARGET_BUILD_APP_PATH%\fonts" /e /i /h /y /d
) else (
    echo [WARN] fonts source folder not found at: %SHADER_SRC%
)

if exist "%TARGET_BUILD_APP_PATH%\winconsole\openscad.com" (
    echo [INFO] Copying winconsole-openscad.com into build directory...
    copy "%TARGET_BUILD_APP_PATH%\winconsole\openscad.com" "%TARGET_BUILD_APP_PATH%\openscad.com"
) else (
    echo [WARN] openscad.com source folder not found at: %SHADER_SRC%
)

echo [INFO] Done. Have a nice day with MSVC-Openscad

