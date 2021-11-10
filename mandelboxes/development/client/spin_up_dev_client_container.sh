#!/bin/bash

# This script spins up the Whist protocol dev client container
# Arguments to this script will be 
# (0) name of this script
# (1) ip address
# (2) port 32262 on the mandelbox
# (3) port 32263 on the mandelbox
# (4) port 32273 on the mandelbox
# (5) AES key
# linux/macos:    ./fclient 35.170.79.124 -p32262:40618.32263:31680.32273:5923 -k 70512c062ff1101f253be70e4cac81bc

# TODO: 0. Find tag of matching Docker image
## App name will be fractal/development/client:current-build; We need to look for images with a tag that matches either
## one of the following regexes [fractal/development/client:current-build:current-build fractal/development/client:current-build]
DOCKER_IMAGE_NAME="fractal/development/client:current-build"
img_lookup_cmd="docker image inspect ${DOCKER_IMAGE_NAME} >/dev/null 2>&1 && echo yes || echo no"

if [[ $(img_lookup_cmd) == "no" ]]; then
	DOCKER_IMAGE_NAME="fractal/development/client:current-build:current-build"
	img_lookup_cmd="docker image inspect ${DOCKER_IMAGE_NAME} >/dev/null 2>&1 && echo yes || echo no"

	if [[ $(img_lookup_cmd) == "no" ]]; then
		echo "Error, development/client image not found!"
		exit
	fi
fi

# TODO: 1. Download any necessary user configs onto the container
# TODO: 2. Apply port bindings/forwarding if needed
# TODO: 3. Pass config variables such as FRACTAL_AES_KEY, which will be saved to file by the startup/entrypoint.sh script, in order for the container to be able to access them later and exported them as environment variables by the `run-fractal-client.sh` script. These config variables will have to be passed as parameters to the FractalClient executable, which will run in non-root mode in the container (username = fractal).
# TODO: 4. Create the Docker container, and start it
docker create -e SERVER_IP_ADDRESS=${1} -e SERVER_PORT_32262=${2} -e SERVER_PORT_32263=${3} -e SERVER_PORT_32273=${4} -e FRACTAL_AES_KEY=${5} $DOCKER_IMAGE_NAME 
docker run $DOCKER_IMAGE_NAME
# TODO: 5. Decrypt user configs within the docker container, if needed
# TODO: 6. Write the config.json file if we want to test JSON transport related features
docker exec $DOCKER_IMAGE_NAME bash -c "touch config.json"
# TODO: 7. Write the .ready file to trigger `base/startup/fractal-startup.sh`
docker exec $DOCKER_IMAGE_NAME bash -c "echo .ready >> .ready"




##### Host Service output when running browsers/chrome for reference
#2021/11/10 14:57:57.042088 SpinUpMandelbox(): spinup started for mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912
#2021/11/10 14:57:57.042104 SpinUpMandelbox(): The APP NAME is fractal/browsers/chrome:current-build
#2021/11/10 14:57:57.042213 SpinUpMandelbox(): created Mandelbox object 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912
#2021/11/10 14:57:57.042221 SpinUpMandelbox(): Successfully assigned mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912 to user localdev_host_service_user_gabriele-dev
#2021/11/10 14:57:57.042266 SpinUpMandelbox(): Beginning user config download for mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912
#2021/11/10 14:57:57.042270 TTY: allocated tty 36
#2021/11/10 14:57:57.042337 Starting S3 config download for mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912
#2021/11/10 14:57:57.042357 SpinUpMandelbox(): successfully allocated TTY 36 and assigned GPU 0 for mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912
#2021/11/10 14:57:57.042399 SpinUpMandelbox(): successfully assigned port bindings [{32262 21000  tcp} {32263 15827  udp} {32273 8150  tcp}]
#2021/11/10 14:57:57.042548 Fetching head object for bucket: fractal-user-app-configs, key: localdev_host_service_user_gabriele-dev/localdev/fractal/browsers/chrome:current-build/fractal-app-config.tar.lz4.enc
#2021/11/10 14:57:57.089001 Using user config version DB.r9GeI1kHxkCIRcMw5nnrGgZnz0OWM for mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912
#2021/11/10 14:57:57.641505 Downloaded 41180480 bytes from s3 for mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912
#2021/11/10 14:57:57.646674 SpinUpMandelbox(): successfully initialized uinput devices.
#2021/11/10 14:57:57.646704 SpinUpMandelbox(): About to look up image for APP NAME fractal/browsers/chrome:current-build using regex [fractal/browsers/chrome:current-build:current-build fractal/browsers/chrome:current-build]
#2021/11/10 14:57:57.646852 Successfully created unix socket at: /fractal/temp/932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912/sockets/uinput.sock
#2021/11/10 16:08:51.550341 SpinUpMandelbox(): found image: fractal/browsers/chrome:current-build
#2021/11/10 14:57:57.834851 dockerevent: create for container d85dabe170137fbb7cf6ac9c1da2493276c4ae2de4aa5cd230da8243bbe18e2f
#2021/11/10 14:57:57.834966 SpinUpMandelbox(): Value returned from ContainerCreate: container.ContainerCreateCreatedBody{ID:"d85dabe170137fbb7cf6ac9c1da2493276c4ae2de4aa5cd230da8243bbe18e2f", Warnings:[]string{}}
#2021/11/10 14:57:57.834979 SpinUpMandelbox(): Successfully ran `create` command and got back runtime ID d85dabe170137fbb7cf6ac9c1da2493276c4ae2de4aa5cd230da8243bbe18e2f
#2021/11/10 14:57:57.835015 SpinUpMandelbox(): Successfully registered mandelbox creation with runtime ID d85dabe170137fbb7cf6ac9c1da2493276c4ae2de4aa5cd230da8243bbe18e2f and AppName fractal/browsers/chrome:current-build
# 2021/11/10 14:57:57.837396 Wrote data "21000" to file hostPort_for_my_32262_tcp
# 2021/11/10 14:57:57.839807 Wrote data "36" to file tty
# 2021/11/10 14:57:57.842089 Wrote data "0" to file gpu_index
# 2021/11/10 14:57:57.843945 Wrote data "-1" to file timeout
# 2021/11/10 14:57:57.843953 SpinUpMandelbox(): Successfully wrote parameters for mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912
# 2021/11/10 14:57:58.313706 dockerevent: start for container d85dabe170137fbb7cf6ac9c1da2493276c4ae2de4aa5cd230da8243bbe18e2f
# 2021/11/10 14:57:58.313803 SpinUpMandelbox(): Successfully started mandelbox fractal-browsers-chrome-current-build-932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912 with ID 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912
# 2021/11/10 14:57:58.313836 SpinUpMandelbox(): Waiting for config encryption token from client...
# 2021/11/10 14:57:58.313846 Decrypting user config for mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912
# 2021/11/10 14:57:58.313864 SpinUpMandelbox(): Successfully downloaded user configs for mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912
# 2021/11/10 14:57:58.395757 Finished decrypting user config for mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912
# 2021/11/10 14:57:58.395777 Decompressing user config for mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912
# 2021/11/10 14:57:58.558991 Untarred config to: /fractal/932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912/userConfigs/unpacked_configs, total size was 64901352 bytes
# 2021/11/10 14:57:58.579570 Writing JSON transport data to config.json file...
# 2021/11/10 14:57:58.586930 Wrote data "" to file config.json
# 2021/11/10 14:57:58.590705 Wrote data ".ready" to file .ready
# 2021/11/10 14:57:58.590715 SpinUpMandelbox(): Successfully marked mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912 as ready
# 2021/11/10 14:57:58.590726 SpinUpMandelbox(): Successfully marked mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912 as running
# 2021/11/10 14:57:58.590741 SpinUpMandelbox(): Finished starting up mandelbox 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912
# 2021/11/10 14:57:58.734932 dockerevent: exec_create: /bin/bash  for container d85dabe170137fbb7cf6ac9c1da2493276c4ae2de4aa5cd230da8243bbe18e2f
# 2021/11/10 14:57:58.735406 dockerevent: exec_start: /bin/bash  for container d85dabe170137fbb7cf6ac9c1da2493276c4ae2de4aa5cd230da8243bbe18e2f
# 2021/11/10 14:57:59.884192 Sent uinput file descriptors to socket at /fractal/temp/932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912/sockets/uinput.sock
# 2021/11/10 14:57:59.884233 SendDeviceFDsOverSocket returned successfully for MandelboxID 932ab22511cd989e2ab72e65c11bb3e1d5514f3ca4613f3a545b52618912