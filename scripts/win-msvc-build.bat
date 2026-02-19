rem Non Manifest Mode Installation (uncomment the line below)
rem vcpkg install boost eigen3 cgal harfbuzz fontconfig double-conversion opencsg libxml2 libzip glib gperf tbb cairo

mkdir build
rem Change the option CMAKE_TOOLCHAIN_FILE to your installation. Below are 2 initial ways to build.
rem 1) Headless Build
cmake -B build -S . -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DHEADLESS=ON -DUSE_BUILTIN_OPENCSG=TRUE -DCMAKE_EXE_LINKER_FLAGS="/manifest:no" -DCMAKE_MODULE_LINKER_FLAGS="/manifest:no" -DCMAKE_SHARED_LINKER_FLAGS="/manifest:no" -DTBB_DIR=%VCPKG_ROOT%/installed/x64-windows/share/tbb
rem 2) GUI Build (Needs Qt and QScintilla Installed from Source, not vcpkg)
rem cmake -B build -S . -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT%/scripts/buildsystems/vcpkg.cmake -DUSE_BUILTIN_OPENCSG=TRUE -DUSE_QT6=ON -DCMAKE_EXE_LINKER_FLAGS="/manifest:no" -DCMAKE_MODULE_LINKER_FLAGS="/manifest:no" -DCMAKE_SHARED_LINKER_FLAGS="/manifest:no" -DTBB_DIR=%VCPKG_ROOT%/installed/x64-windows/share/tbb -Dunofficial-qscintilla_DIR="%VCPKG_ROOT%/installed/x64-windows/share/unofficial-qscintilla"

rem Second step of build process, each build should take ~30 mins for a Headless build.
cmake --build build --config Debug
cmake --build build --config Release