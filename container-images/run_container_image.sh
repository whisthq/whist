#!/bin/bash

set -Eeuo pipefail

image=${1:-fractal/base:current-build}
if [[ ${2:-''} == mount ]]; then
    mount_protocol="--mount type=bind,source=$(cd base/protocol/server/build64;pwd),destination=/usr/share/fractal/bin"
else
    mount_protocol=""
fi

runcontainer() {
    docker run -it -d \
	    -v /sys/fs/cgroup:/sys/fs/cgroup:ro \
	    -v /fractal:/fractal:ro \
        $mount_protocol \
	    --tmpfs /run \
	    --tmpfs /run/lock \
	    --gpus all \
	    -e NVIDIA_CONTAINER_CAPABILITIES=all \
	    -e NVIDIA_VISIBLE_DEVICES=all \
	    --shm-size=8g \
        --cap-drop ALL \
        --cap-add CAP_SETPCAP \
        --cap-add CAP_MKNOD \
        --cap-add CAP_AUDIT_WRITE \
        --cap-add CAP_CHOWN \
        --cap-add CAP_NET_RAW \
        --cap-add CAP_DAC_OVERRIDE \
        --cap-add CAP_FOWNER \
        --cap-add CAP_FSETID \
        --cap-add CAP_KILL \
        --cap-add CAP_SETGID \
        --cap-add CAP_SETUID \
        --cap-add CAP_NET_BIND_SERVICE \
        --cap-add CAP_SYS_CHROOT \
        --cap-add CAP_SETFCAP \
        --cap-add SYS_NICE \
        -p 32262:32262 \
        -p 32263:32263/udp \
        -p 32273:32273 \
        $image
# capabilities not enabled by default: CAP_NICE
}

container_id=$(runcontainer $1)

echo "Running container with ID: $container_id"
docker exec -it $container_id /bin/bash
docker kill $container_id
docker rm $container_id
