cmake_minimum_required(VERSION 3.15)
project(DesktopCaptureEncode)

set(CMAKE_CXX_STANDARD 14)
LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}") # append FindDirectX11.cmake to module path

# Cuda
find_package(CUDA 9.0 REQUIRED)
include_directories("${CUDA_INCLUDE_DIRS}")

# DirectX11
find_package(DirectX11 REQUIRED)
include_directories(${DirectX11_INCLUDE_DIRS})

include_directories(nvEncDXDDI)
include_directories(NvEncoder)

add_executable(DesktopCaptureEncode
        nvEncDXDDI/DDAImpl.cpp
        nvEncDXDDI/DDAImpl.h
        nvEncDXDDI/Defs.h
        nvEncDXDDI/main.cpp
        nvEncDXDDI/Preproc.cpp
        nvEncDXDDI/Preproc.h
        NvEncoder/nvEncodeAPI.h
        NvEncoder/NvEncoder.cpp
        NvEncoder/NvEncoder.h
        NvEncoder/NvEncoderD3D11.cpp
        NvEncoder/NvEncoderD3D11.h)