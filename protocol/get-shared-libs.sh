#!/bin/bash
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/shared-libs.tar.gz" | tar xz


mkdir -p desktop/build32/
mkdir -p server/build32/
mkdir -p desktop/build64/
mkdir -p server/build64/
# in case we want to target Windows
cp share/32/Windows/* desktop/build32/
cp share/32/Windows/* server/build32/
cp share/64/Windows/* desktop/build64/
cp share/64/Windows/* server/build64/

echo "done"
