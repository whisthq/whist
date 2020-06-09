#!/bin/bash

if [ "$(uname)" == "Darwin" ]; then
  curl -s -O "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/mac_unison"
  mkdir -p desktop/external_utils/Darwin
  mv mac_unison desktop/external_utils/Darwin
else
  curl -s -O "https://fractal-cloud-setup-s3bucket.s3.amazonaws.com/linux_unison"
  mkdir -p desktop/external_utils/Linux
  mv linux_unison desktop/external_utils/Linux
fi

echo "done"
