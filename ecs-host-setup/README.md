# Fractal ECS Host Setup

This repository contains the scripts to set up an AWS EC2 host machine to host Fractal containers. The `setup_ubuntu20_host.sh` script is intended for setting up a general host for development, while the `setup_ubuntu20_ami_host.sh` script sets up an EC2 host and stores it as an AMI (Amazon Machine Image) for programmatic deployment. To use either of the scripts:

- First, create an EC2 Instance (which will later be linked with ECS), you need to create a Ubuntu 20.04 instance on a g3s.xlarge instance type
- Also, make sure to add more storage space (the default 8g is not enough to build the protocol and the base image!). 32 is plenty, 16 should be enough as well.
- Also, make sure that it is part of the security group “fractal-containerized-protocol-group”, on us-east-2, or “container-testing”, on us-east-1, on “Step 6: Configure Security Group”

Then, run the following commands on the host:

```
curl https://raw.githubusercontent.com/fractalcomputers/ecs-host-setup/master/setup_ubuntu20_host.sh?token=AGNK4MEXEJSKU5TOI3U7HAS7RBVWI > setup_host.sh
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

Now from a fractal client, try connecting to the IP given by `curl ipinfo.io` inside the container.


