#!/bin/bash

runcontainer (){
    docker run -it -d --privileged -v /sys/fs/cgroup:/sys/fs/cgroup:ro --mount type=bind,source=$(cd $2;pwd),destination=/protocol -p 5900:5900 -p 32262:32262 -p 32263:32263 -p 32264:32264 -p 80:80 -p 22:22 -p 443:443 -e VNC_SERVER_PASSWORD=password -t chrome-systemd-$1
}

docker build -f chrome/Dockerfile.$1 chrome -t chrome-systemd-$1
container_id=$(runcontainer $1 $2)
# docker exec -u 0 $container_id /bin/bash -c "source /utils.sh && Enable-FractalFirewallRule"
# docker exec -u 0 $container_id /bin/bash -c "yes | ufw allow 5900"
# docker cp ../docker-systemctl-replacement/files/docker/systemctl.py

echo "Running chrome in container: $container_id"
ipaddr=$(docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' $container_id)
echo "Chrome is running at IP address $ipaddr"
docker exec -it $container_id /bin/bash
docker kill $container_id
