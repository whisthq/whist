# Fractal ECS Host Setup

This subfolder contains the scripts needed to help you set up an EC2 development instance for developing Fractal, and for setting up an EC2 AMI (Amazon Machine Image), which is a fixed operating system image which is deployed on our production EC2 instances. Generating AMIs and using them in production result in faster deployment, since the AMIs are pre-created, and ensure that all of our production EC2 instances run the same exact operating system.

This repository contains the scripts to set up an AWS EC2 host machine to host Fractal containers. The `setup_ubuntu20_host.sh` script is intended for setting up a general host for development, while the `setup_ubuntu20_ami_host.sh` script sets up an EC2 host and stores it as an AMI (Amazon Machine Image) for programmatic deployment. To use either of the scripts:

First, create an Ubuntu Server 20.04 g3s.xlarge EC2 instance on AWS region **us-east-1**. This instance will later be linked with ECS. The **g3** instance type is required for GPU compatibility with our containers and streaming technology. Before lauching, make sure to select at least 32GB of storage space in the "Add Storage" section, as the default 8GB is not enough to build the protocol and the base image. Also before launching, add your EC2 instance to the security groups **container-testing** and **Open ECS Host Service Port**.

While we recommend that you set up your dev instance on AWS region **us-east-1**, it is possible to use a different region. You will have to manually ensure that the relevant ports (covered by the aforementioned security groups in **us-east-1**) are opened on your instance. For example, on AWS region **us-east-2**, you can apply the **fractal-containerized-protocol-group**, but you will have to manually open up TCP port 4678 on your instance. Instructions will vary per-region.

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

If you are on a high-DPI screen, you can optionally prepend the final line of the above code block with `FRACTAL_DPI=250` (or any other value) to override the default DPI value of 96 for the container.

Now, try starting a Fractal client to connect to the Fractal server by following the instructions in `protocol/desktop/README.md`. If a window pops up that streams xterm/whatever the base application is, then you are set!

## Copying AWS AMIs Across Regions

If you have created an AMI in a specific AWS region (i.e. `us-east-1`) which you would like to replicate in a different AWS region (i.e. `us-west-1`), you can either re-run the scripts in a different region and start the process from scratch, or you can copy over your image. For complete details on how to do this process, see our [ECS Documentation on Notion](https://www.notion.so/tryfractal/4d91593ea0e0438b8bdb14c25c219d55?v=0c3983cf062d4c3d96ac2a65eb31761b&p=ca4fdec782894072a6dd63f32b494e1d).

## Design Decisions

### Host Service Controlling the ECS Agent

We have made the decision to make the host service start up the Docker daemon when it is ready (i.e. after the handshake with the webserver is complete). Previously, the Docker daemon and ECS agent would start up on host machine boot, and our host service would have to race to initialize before the host became marked as ready to accept new tasks. Now, we start the Docker daemon and the ECS agent only when we're ready to process Docker events, and we guarantee that the host service never misses the startup of a container. Furthermore, by waiting for the authentication handshake with the webserver to complete before starting the ECS agent, we ensure that a host that failed to authenticate (and therefore fails to deliver heartbeats, thus getting killed by the webserver in a few minutes) does not accept any tasks.

**NOTE**: If you want to see the actual userdata that gets passed into the EC2 hosts, it's in the subfolder `main-webserver` under the [`fractal/fractal`](https://github.com/fractal/fractal) repository; it is the file [`app/helpers/utils/aws/base_userdata_template.sh`](https://github.com/fractal/fractal/blob/master/main-webserver/app/helpers/utils/aws/base_userdata_template.sh).
