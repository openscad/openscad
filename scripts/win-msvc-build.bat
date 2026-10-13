rem Architecture specific definitions: vcpkg triplet, Visual Studio generator and target arch.
if /i "%PROCESSOR_ARCHITECTURE%"=="ARM64" (
  set ARCH=ARM64
  set TRIPLET=arm64-windows
  set GENERATOR=Visual Studio 18 2026
) else (
  set ARCH=x64
  set TRIPLET=x64-windows
  set GENERATOR=Visual Studio 17 2022
)

rem Non Manifest Mode Installation (uncomment the line below)
rem vcpkg install --triplet %TRIPLET% boost eigen3 cgal harfbuzz fontconfig double-conversion opencsg libxml2 libzip glib gperf tbb cairo

mkdir build
rem Change the option CMAKE_TOOLCHAIN_FILE to your installation. Below are 2 initial ways to build.
rem 1) Headless Build
cmake -B build -S . -G "%GENERATOR%" -A %ARCH% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=%TRIPLET% -DHEADLESS=ON -DUSE_BUILTIN_OPENCSG=TRUE -DCMAKE_EXE_LINKER_FLAGS="/manifest:no" -DCMAKE_MODULE_LINKER_FLAGS="/manifest:no" -DCMAKE_SHARED_LINKER_FLAGS="/manifest:no" -DTBB_DIR=%VCPKG_ROOT%/installed/%TRIPLET%/share/tbb
rem 2) GUI Build (Needs Qt and QScintilla Installed from Source, not vcpkg)
rem cmake -B build -S . -G "%GENERATOR%" -A %ARCH% -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DVCPKG_TARGET_TRIPLET=%TRIPLET% -DUSE_BUILTIN_OPENCSG=TRUE -DUSE_QT6=ON -DCMAKE_EXE_LINKER_FLAGS="/manifest:no" -DCMAKE_MODULE_LINKER_FLAGS="/manifest:no" -DCMAKE_SHARED_LINKER_FLAGS="/manifest:no" -DTBB_DIR=%VCPKG_ROOT%/installed/%TRIPLET%/share/tbb -Dunofficial-qscintilla_DIR="%VCPKG_ROOT%/installed/%TRIPLET%/share/unofficial-qscintilla"

rem Second step of build process, each build should take ~30 mins for a Headless build.
cmake --build build --config Debug
cmake --build build --config Release