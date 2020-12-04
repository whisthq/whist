#!/bin/bash
docker build . \
             --build-arg uid=$(id -u ${USER}) \
             -f Dockerfile \
             -t fractal/protocol-builder
