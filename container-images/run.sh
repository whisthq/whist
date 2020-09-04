#!/bin/bash

runcontainer (){
    docker run -it -d \
	    -v /sys/fs/cgroup:/sys/fs/cgroup:ro \
	    --tmpfs /run \
	    --tmpfs /run/lock \
	    --gpus all \
	    -e NVIDIA_CONTAINER_CAPABILITIES=all \
	    -e NVIDIA_VISIBLE_DEVICES=all \
	    --privileged \
	    --shm-size=8g \
	    --security-opt seccomp=unconfined \
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
            --mount type=bind,source=$(cd $3;pwd),destination=/protocol -p 32262:32262 -p 32263:32263/udp -p 32273:32273 $1-systemd-$2
}

docker build -f $1/Dockerfile.$2 $1 -t $1-systemd-$2
container_id=$(runcontainer $1 $2 $3)

echo "Running container with IP: $container_id"
ipaddr=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $container_id)
echo "Container is running with IP address $ipaddr"
docker exec -it $container_id /bin/bash
docker kill $container_id
docker rm $container_id
