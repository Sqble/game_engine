@echo off
setlocal

REM Set PATH to use 64-bit MinGW-w64
set PATH=C:\msys64\mingw64\bin;%PATH%

set BUILD_DIR="build"
set BUILD_TYPE="Debug"

if "%CLEAN_BUILD%"==true (
  rmdir /s /q %BUILD_DIR%
  mkdir %BUILD_DIR%
)

if not exist %BUILD_DIR% mkdir %BUILD_DIR%

pushd %BUILD_DIR%

conan install .. --output-folder=. --build=missing --settings=build_type=%BUILD_TYPE% --settings=compiler=gcc --settings=compiler.version=15 --settings=arch=x86_64 --settings=os=Windows 

cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
copy compile_commands.json ..
cmake --build .

REM Run the executable from the correct location
if exist game.exe (
    game.exe
) else (
    echo game.exe not found in Debug folder.
)

popd

endlocal