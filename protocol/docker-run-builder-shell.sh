#!/bin/bash
container_name=protocol-builder-$(date +"%s")
mount_directory=$(cd ${1:-.}; pwd)
username=$(id -u ${USER})
command=${2:-'/bin/bash'}
docker run -it \
    --rm \
    --mount type=bind,source=$mount_directory,destination=/workdir \
    --name $container_name \
    --user $username \
    protocol-builder-ubuntu20 \
    sh -c "$command"
