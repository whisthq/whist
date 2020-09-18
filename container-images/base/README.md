# Base Container Image

![Docker Image CI](https://github.com/fractalcomputers/container-images/workflows/Docker%20Image%20CI/badge.svg)

This folder contains the container image for a basic container streaming Fractal. The Dockerfile is based on an Ubuntu 18.04 image and installs all required dependencies besides the protocol itself.

For reference on the bash functions called in the Docker image, refer to: https://github.com/fractalcomputers/setup-scripts

For reference on the Fractal protocol and streaming, refer to: https://github.com/fractalcomputers/protocol

## Architecture

The container (specified by Dockerfile.18, Dockerfile.20 is deprecated) is oriented around systemd, which is the entrypoint to the container. To specify what code actually runs in the container after startup, at buildtime we copy over a set of systemd services that, when systemd runs, will do some runtime configuration and start all the processes that we need.

## Running

Run with `./run.sh base 18 /your/protocol/directory` from `/container-images`

## Services

The services `fractal-entrypoint.service`, `fractal-display.service`, and `fractal-protocol.service` must run successfully for the container to be functional. These services run in the order written above and are run by systemd on container start. 

`fractal-entrypoint.service` runs the contents of the `entry.sh` script as root -- this is where configuration that has to occur during runtime can normally go. 

`fractal-display.service` starts an X server with the proper configuration that we need -- see display-service-envs for environment variables imported in the service.

`fractal-protocol.service` runs the protocol and configures some environment variables so it works correctly with the X server

## Useful Debugging Practices
`systemctl status` shows all the running services in the container. To be more precise, if you run it inside the container, you'll see all the services running in the container and the subprocesses they've started etc. If you run it outside the container on a linux machine, you'll see all the host services mixed in together with the container's services. 

`journalctl -e` shows the end of the systemd logs (you can page further backwards by hitting `b`).

`journalctl` shows all the systemd logs. Too much to read unless you pipe it through grep

`/usr/share/fractal/log.txt` fractal protocol logs. They can also be found in the systemd logs with `journalctl | grep FractalServer`.

`/testing-scripts` contains some little test scripts that can help debug problems on the host



### Ignore the old info below, but it will change

First, start the container and the Fractal server:

```
docker run -p 5900:5900 --name chrome --user fractal --privileged <image-name>
```

Once the container is running, you can use Fractal to go into it and run Chrome
from a terminal window by running:
```
google-chrome
```

You can also start Google Chrome by right-clicking the Desktop and selecting:
```
Applications > Network > Web Browsing > Google Chrome
```
