# Fractal Base Container Image

This subfolder contains the base container Dockerfile that builds the core Fractal container running Fractal and general utilities and services that application-specific Dockerfiles can build upon. The base image runs **Ubuntu 20.04**.

This subfolders conforms to the **Styling** and **Continous Integration** practices defined in the outer README.

## Development

This repository calls the Fractal [Protocol](https://github.com/fractal/protocol) repository as a Git submodule for integrating our streaming technology in the Dockerfile. Refer to the outer README for instructions on fetching the protocol Git submodule and building it into the Dockerfile.

Any Linux services needed for the containers to function properly should be developed and hosted as part of this project subfolder. For any details on continuous integration and styling, refer to the outer README of this project.

### Architecture

The base container, specified by Dockerfile.20, is oriented around **systemd**, which is the entrypoint to the container. To specify what code actually runs in the container after startup, we copy over a set of systemd services at buildtime that, when systemd runs, will do some runtime configuration and start all the processes that we need.

### Services

The services `fractal-entrypoint.service`, `fractal-display.service`, `fractal-audio.service` and `fractal-protocol.service` must run successfully for the container to be functional. These services run in the order written above and are run by systemd on container start. 

`fractal-entrypoint.service` runs the contents of the `entry.sh` script as root -- this is where configuration that has to occur during runtime can normally go. 

`fractal-display.service` starts an X Server with the proper configuration that we need. Note that this starts an X Server that is powered by an Nvidia GPU, meaning our containers can only be run on GPU-powered hosts.

`fractal-audio.service` starts a virtual Pulse Audio soundcard in the container.

`fractal-protocol.service` runs the protocol and configures some environment variables so that it works correctly with the X Server.

### Useful Debugging Practices

You can run `systemctl status` to see all the running services in the container. To be more precise, if you run it inside the container, you'll see all the services running in the container and the subprocesses they've started. If you run it outside the container on a Linux host machine, you'll see all the host services mixed in together with the container's services. 

`journalctl -e` shows the end of the systemd logs (you can page further backwards by hitting `b`).

`journalctl` shows all the systemd logs. Too much to read unless you pipe it through grep.

`/usr/share/fractal/log.txt` contains the Fractal protocol logs. They can also be found in the systemd logs with `journalctl | grep FractalServer`.

`/testing-scripts` contains some little test scripts that can help debug problems on the host machine.

## Running

See **Running Local Images** and **Running Remote-Pushed Images** in the outer README.
