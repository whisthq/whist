#!/bin/bash
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/shared-libs.tar.gz" | tar xz

# in case we want to target Windows
cp share/32/Windows/* desktop/build32/
cp share/32/Windows/* desktop/build32/
cp share/64/Windows/* desktop/build64/
cp share/64/Windows/* desktop/build64/
