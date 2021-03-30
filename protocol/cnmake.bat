REM Script to build the Fractal protocol on Windows

@echo off
cd %~dp0
mkdir build
cmake -S . -B build/ -G Ninja -D CMAKE_BUILD_TYPE=Debug
