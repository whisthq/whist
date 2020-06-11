#!/bin/bash

# Get shared libs
curl -s "https://fractal-protocol-shared-libs.s3.amazonaws.com/shared-libs.tar.gz" | tar xz

mkdir -p desktop/build32/Windows
mkdir -p server/build32/Windows
mkdir -p desktop/build64/Windows
mkdir -p server/build64/Windows

# in case we want to target Windows
cp share/32/Windows/* desktop/build32/Windows/
cp share/32/Windows/* server/build32
cp share/64/Windows/* desktop/build64/Windows/
cp share/64/Windows/* server/build64

# Get Unison binary
if [ "$(uname)" == "Darwin" ]; then
  curl -s -O "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/mac_unison"
  mkdir -p desktop/external_utils/Darwin
  mv mac_unison desktop/external_utils/Darwin
else
  curl -s -O "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/linux_unison"
  mkdir -p desktop/external_utils/Linux
  mv linux_unison desktop/external_utils/Linux
fi

# Get loading bitmaps
curl -s "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/loading.tar.gz" | tar xz
rm -rf desktop/loading
mv loading desktop

echo "done"
