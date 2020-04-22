#!/bin/bash          
bytes=64
rndbytes=hexdump -n 64 -e '4/4 "%08X" 1 "\n"' /dev/random
EncodedText= $rndbytes

version=$EncodedText

aws s3 cp server.exe s3://arn:aws:s3:us-east-1:747391415460:accesspoint/fractal-cloud-setup/FractalServer
aws s3 cp version s3://arn:aws:s3:us-east-1:747391415460:accesspoint/fractal-cloud-setup/version

type version
