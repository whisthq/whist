# Fractal ECS Host Setup

This repository contains the scripts to set up an AWS EC2 host machine to host Fractal containers. The `setup_ubuntu20_host.sh` script is intended for setting up a general host for development, while the `setup_ubuntu20_ami_host.sh` script sets up an EC2 host and stores it as an AMI (Amazon Machine Image) for programmatic deployment. To use either of the scripts:

First, create an Ubuntu Server 20.04 g3s.xlarge EC2 instance (which will later be linked with ECS) - the **g3** instance type is required for GPU compatibility with our containers and streaming technology. Before lauching, make sure to select 32GB of storage space in the "Add Storage" section, as the default 8GB is not enough to build the protocol and the base image. Also before launching, add your EC2 instance to the the security group **fractal-containerized-protocol-group** if on AWS region **us-east-2**, or **container-testing** if on AWS region **us-east-1** in the "Security Groups" section.

Then, launch the instance. Create a new key pair for the instance and make sure to save the `.pem` file, as it's required to SSH into the instance. After the EC2 instance boots up, SSH into it. Install `go` ([instructions](https://linuxize.com/post/how-to-install-go-on-ubuntu-20-04/)), then run the following commands:

```
git clone --branch dev https://github.com/fractal/fractal.git
cd ~/fractal/ecs-host-setup
./setup_ubuntu20_host.sh
sudo reboot
cd ~/fractal/container-images
git submodule init
git submodule update
cd ~/fractal/protocol/
./build_protocol.sh
cd ~/fractal/container-images
./build_container_image.sh base
cd ~/fractal/ecs-host-service
make run # keep this open in a separate terminal
cd ~/fractal/container-images
./run_local_container_image.sh base
```

Now, try starting a Fractal client to connect to the Fractal server by following the instructions in `protocol/desktop/README.md`. You can get the IP address of your EC2 instance by running `curl ipinfo.io` inside the instance. If a window pops up that streams xterm/whatever the base application is, then you are set!

## Design Decisions

### Host Service Controlling the ECS Agent

We have made the decision to make the host service start up the Docker daemon when it is ready (i.e. after the handshake with the webserver is complete). Previously, the Docker daemon and ECS agent would start up on host machine boot, and our host service would have to race to initialize before the host became marked as ready to accept new tasks. Now, we start the Docker daemon and the ECS agent only when we're ready to process Docker events, and we guarantee that the host service never misses the startup of a container. Furthermore, by waiting for the authentication handshake with the webserver to complete before starting the ECS agent, we ensure that a host that failed to authenticate (and therefore fails to deliver heartbeats, thus getting killed by the webserver in a few minutes) does not accept any tasks.

**NOTE**: If you want to see the actual userdata that gets passed into the EC2 hosts, it's in the repository [`main-webserver`](https://github.com/fractal/main-webserver); it is the file [`app/helpers/utils/aws/base_userdata_template.sh`](https://github.com/fractal/main-webserver/blob/master/app/helpers/utils/aws/base_userdata_template.sh).
