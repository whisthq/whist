# Fractal ECS Host Setup

This repository contains the scripts to set up an AWS EC2 host machine to host Fractal containers. The `setup_ubuntu20_host.sh` script is intended for setting up a general host for development, while the `setup_ubuntu20_ami_host.sh` script sets up an EC2 host and stores it as an AMI (Amazon Machine Image) for programmatic deployment. To use either of the scripts:

First, create an Ubuntu 20.04 g3s.xlarge EC2 instance (which will later be linked with ECS) - the **g3** instance type is required for GPU compatibility with our containers and streaming technology. Make sure to select 32GB of storage space, as the default 8GB is not enough to build the protocol and the base image, and to add your EC2 insance to the the **fractal-containerized-protocol-group** if on AWS region **us-east-2**, or **container-testing** if on AWS region **us-east-1**.

Then, run the following commands on the EC2 instance, via AWS Session Manager (SSM):

```
curl https://raw.githubusercontent.com/fractalcomputers/ecs-host-setup/master/setup_ubuntu20_host.sh?token=AGNK4MD4UVYRUQR4425K64C7TN3MU > setup_host.sh
chmod +x setup_host.sh
./setup_host.sh
sudo reboot
git clone https://github.com/fractalcomputers/container-images
cd container-images/
git checkout dev
git submodule init
git submodule update
cd base/protocol/
git checkout dev
cd ../..
./build_protocol.sh && ./build_container_image.sh base && ./run_container_image.sh base
```

Now, from a Fractal client, try connecting to the IP given by running `curl ipinfo.io` inside the container. You should be all set!
