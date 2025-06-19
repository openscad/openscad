@echo off
setlocal enabledelayedexpansion

mkdir build

rem Modify toolchain to your vcpkg installation
cmake -B build -S . ^
    -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake ^
    -DVCPKG_TARGET_TRIPLET=x64-windows ^
    -DUSE_BUILTIN_OPENCSG=TRUE ^
    -DENABLE_CAIRO=FALSE ^
    -DHEADLESS=ON ^
    -DCMAKE_EXE_LINKER_FLAGS="/manifest:no" ^
    -DCMAKE_MODULE_LINKER_FLAGS="/manifest:no" ^
    -DCMAKE_SHARED_LINKER_FLAGS="/manifest:no" ^
    -G "Visual Studio 17 2022" ^
    -A x64

rem Currently, only Debug builds work completly. Hence, the Release build is commented out.
cmake --build build --config Debug
rem cmake --build build --config Release