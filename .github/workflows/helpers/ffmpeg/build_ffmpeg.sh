#!/bin/bash

# Configure and Build FFmpeg on Linux Ubuntu with relevant flags to build shared libs. We use `--enable-nonfree` and
# `--enable-gpl`, since they are required by `--enable-cuda-nvcc`, `--enable-libx264`, and `--enable-libx265`, which
# is required for the FFmpeg software encoder tests we run. This is compliant with the license since we don't distribute
# a Linux client, and only install those libs on our servers.

cd FFmpeg
./configure \
--arch=x86_64 \
--extra-cflags=-O2 \
--disable-programs \
--disable-doc \
--disable-debug \
--disable-sdl2 \
--enable-gpl \
--enable-nonfree \
--enable-cuda-nvcc \
--enable-libx264 \
--enable-libx265 \
--enable-libopus \
--enable-nvenc \
--enable-libmfx \
--enable-pthreads \
--enable-filter=scale_cuda \
--disable-static \
--enable-shared

# Build FFmpeg and move static/shared libs
make -j && rm -rf linux-build && mkdir linux-build
find ./ -type f \( -iname \*.so -o -iname \*.a \) | xargs -I % cp % linux-build/
make install && ldconfig
