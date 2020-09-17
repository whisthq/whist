#!/bin/bash
container_name=protocol-builder-$(date +"%s")
docker run -it --rm --mount type=bind,source=$(pwd),destination=/workdir --name $container_name \
            --user $(id -u ${USER}) protocol-builder-ubuntu$1
