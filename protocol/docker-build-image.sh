#!/bin/bash
docker build --build-arg uname=$(whoami) --build-arg uid=$(id -u ${USER}) \
            --build-arg gid=$(id -g ${USER}) -f Dockerfile.builder.$1 \
            . -t protocol-builder-ubuntu$1