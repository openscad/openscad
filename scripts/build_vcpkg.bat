@echo off
setlocal enabledelayedexpansion

vcpkg install --triplet=x64-windows
mkdir build
rem Modify toolchain to your vcpkg installation directory
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

cmake --build build --config Debug
cmake --build build --config Release