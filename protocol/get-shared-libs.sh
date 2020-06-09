#!/bin/bash
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/share.tar.gz" | tar xz


mkdir -p desktop/build32/Windows
mkdir -p server/build32/Windows
mkdir -p desktop/build64/Windows
mkdir -p server/build64/Windows
mkdir -p server/build64/Darwin

# in case we want to target Windows
cp share/32/Windows/* desktop/build32/Windows/
cp share/32/Windows/* server/build32/Windows/
cp share/64/Windows/* desktop/build64/Windows/
cp share/64/Windows/* server/build64/Windows/
cp share/64/Darwin/* server/build64/Darwin/
echo "done"
