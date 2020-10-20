#!/bin/bash
container_name=protocol-builder-$(date +"%s")
mount_directory=$(cd ${1:-.}; pwd)
username=$(id -u ${USER})
command=${2:-'/bin/bash'}
if [ -z "$2" ]
then
    docker_run_flags=-it
fi
docker run $docker_run_flags \
    --rm \
    --mount type=bind,source=$mount_directory,destination=/workdir \
    --name $container_name \
    --user $username \
    protocol-builder-ubuntu20 \
    sh -c "$command"
