sleep 10s
docker cp $1:/usr/share/fractal/server.log /tmp/server.log
cat /tmp/server.log | grep "Time between" | tr -s ' ' | cut -d' ' -f 17 | sed 's/.$//' | tail -n 5 | jq -s add/length
