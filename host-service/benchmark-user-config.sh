#!/bin/bash

make build
export APP_ENV=localdev
ln -sf /home/ubuntu/.aws /root/.aws
build/host-service &> host_service.log &
HOST_SERVICE_PID=$!

while ! grep "Entering event loop..." host_service.log
do
    sleep 3
done

echo "host service started"
rm timestamps.log

for i in $(seq "$1")
do
    ../mandelboxes/run_local_mandelbox_image.sh browsers/chrome > /dev/null &
    MANDELBOX_PID=$!

    sleep 5

    grep "Starting S3 config download" host_service.log | tail -1 >> timestamps.log
    grep "Ran \"aws s3 cp\" get config command" host_service.log | tail -1 >> timestamps.log

    grep "Starting decryption" host_service.log | tail -1 >> timestamps.log
    grep "Decrypted config to" host_service.log | tail -1 >> timestamps.log

    grep "Starting extraction" host_service.log | tail -1 >> timestamps.log
    grep "Untar config directory output" host_service.log | tail -1 >> timestamps.log

    printf "\n-------------------------\n" >> timestamps.log

    kill $MANDELBOX_PID
    wait $MANDELBOX_PID
done

kill $HOST_SERVICE_PID
wait $HOST_SERVICE_PID
