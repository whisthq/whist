@echo off

REM Script to build the Whist protocol on Windows

REM Move to parent directory of this script
pushd %~dp0

mkdir build
cmake -S . -B build/ -G Ninja -D CMAKE_BUILD_TYPE=Debug
cd build
ninja

popd
