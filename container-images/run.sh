#!/bin/bash

runcontainer (){
    docker run -it -d --cap-add SYS_ADMIN -v /sys/fs/cgroup:/sys/fs/cgroup:ro --mount type=bind,source=$(cd $3;pwd),destination=/protocol -p 32262:32262 -p 32263:32263/udp -p 32273:32273 $1-systemd-$2
}

docker build -f $1/Dockerfile.$2 $1 -t $1-systemd-$2
container_id=$(runcontainer $1 $2 $3)

echo "Running container with IP: $container_id"
ipaddr=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $container_id)
echo "Container is running with IP address $ipaddr"
docker exec -it $container_id /bin/bash
docker kill $container_id
docker rm $container_id
