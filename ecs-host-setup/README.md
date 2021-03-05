# Fractal ECS Host Setup

This subfolder contains the scripts to set up AWS EC2 instances for developing Fractal, and for setting up AWS EC2 AMIs (Amazon Machine Images) for our production EC2 instances running Fractal containers for our users.

An AMI is an operating system image, in our case a snapshot of Linux Ubuntu with specific packages, drivers and settings preinstalled and preconfigured, which can be easily loaded onto an EC2 instance instead of using default Linux Ubuntu. We use Fractal-specific AMIs with our EC2 instances as it results in faster deployment (the images are prebuilt) and ensures that all of our EC2 instances run the exact same operating system, which is specifically optimized for running Fractal containers.

The `setup_ubuntu20_host.sh` script lets you set up a general EC2 instance host for development (by setting up proper NVIDIA drivers and Docker daemon configs and filters), while running `setup_ubuntu20_host.sh` followed by `setup_ubuntu20_ami_host.sh` sets up an EC2 host that can be manually stored as an AMI for programmatic deployment.

## Setting Up a Development Instance

To set up your Fractal development instance:

- Create an Ubuntu Server 20.04 g3s.xlarge or g3.4xlarge EC2 instance on AWS region **us-east-1**, with at least 32 GB of storage (else you will run out of storage for the Fractal protocol and base container image). Note that the **g3** EC2 instance type is required for GPU compatibility with our containers and streaming technology.

- Add your EC2 instance to the security group **container-testing**, to enable proper networking rules. If you decide to set up your EC2 instance in a different AWS region, you will need to add it to the appropriate security group for that region, which may vary per region.

- Create a new keypair and save the `.pem` file as it is required to SSH into the instance, unless you use AWS Session Manager (AWS' version of SSH, accessible from the AWS console). Then, launch the instance.

- SSH/SSM into your instance and install `Go` via the following ([instructions](https://linuxize.com/post/how-to-install-go-on-ubuntu-20-04/)).

- Then, run the following commands:

```
# clones `dev` by default
git clone https://github.com/fractal/fractal.git

# set up the EC2 host for development
cd ~/fractal/ecs-host-setup
./setup_ubuntu20_host.sh
sudo reboot

# build the Fractal protocol server
cd ~/fractal/protocol/
./build_protocol.sh

# build the Fractal base container image
cd ~/fractal/container-images
./build_container_image.sh base

# build the Fractal ECS host service
cd ~/fractal/ecs-host-service
make run # keep this open in a separate terminal

# run the Fractal base container image
cd ~/fractal/container-images
./run_local_container_image.sh base
```

If you are on a high-DPI screen, you can optionally prepend the final line of the above code block with `FRACTAL_DPI=250` (or any other value) to override the default DPI value of 96 for the container.

- Start a Fractal protocol client to connect to the Fractal protocol server running on your instance by following the instructions in [`protocol/desktop/README.md`](https://github.com/fractal/fractal/blob/dev/protocol/desktop/README.md). If a window pops up that streams the Fractal base application, which is currently **xterm**, then you are all set!

## Setting Up an AMI

To create an AMI:

- Create an Ubuntu Server 20.04 g3.4xlarge EC2 instance on AWS region **us-east-1**, with at least 32 GB of storage.

- Add your EC2 instance to the relevant production-ready security group(s) for the region you created it in. You can see which security group(s) currently-running production EC2 instances are attached to in the AWS console.

- Create a new keypair and save the `.pem` file as it is required to SSH into the instance, unless you use AWS Session Manager (AWS' version of SSH, accessible from the AWS console). Then, launch the instance.

- SSH/SSM into your instance and run the following commands:

```
# clones `dev` by default
git clone https://github.com/fractal/fractal.git

# set up the EC2 host with proper packages and drivers,
# and sets Fractal Docker daemon configs from docker-demon-config/daemon.json
cd ~/fractal/ecs-host-setup
./setup_ubuntu20_host.sh
sudo reboot

# set up networking rules and system configs and services
# this script also copies over the userdata-bootstrap.sh script
cd ~/fractal/ecs-host-setup
./setup_ubuntu20_ami_host.sh

# VERY IMPORTANT: remove all Fractal code!
cd ~
rm -rf fractal
```

- Manually test that this instance can be attached to an AWS EC2 instances cluster and that it can be connected to, and then save it as an AMI in the AWS console.

**NOTE**: If you want to see the actual userdata that gets passed into the EC2 hosts, it's in the subfolder `main-webserver` in the file [`app/helpers/utils/aws/base_userdata_template.sh`](https://github.com/fractal/fractal/blob/dev/main-webserver/app/helpers/utils/aws/base_userdata_template.sh).

## Copying AMIs Across AWS Regions

If you have created an AMI in a specific AWS region (i.e. `us-east-1`) which you would like to replicate in a different AWS region (i.e. `us-west-1`), you can either re-run the scripts in a different region and start the process from scratch, or you can copy over your AMI (which is much faster). For complete details on how to copy over AMIs, see our [ECS Documentation on Notion](https://www.notion.so/tryfractal/4d91593ea0e0438b8bdb14c25c219d55?v=0c3983cf062d4c3d96ac2a65eb31761b&p=ca4fdec782894072a6dd63f32b494e1d).

## Design Decisions

### Fractal Host Service Controlling the ECS Agent

We have made the decision to make the Fractal ECS host service start up the Docker daemon when it is ready (i.e. after the handshake with the webserver is complete). The ECS Agent is a system service that comes pre-installed on ECS-enabled EC2 instances and is responsible for orchestrating and managing Docker events and containers on the instance, on behalf of AWS.

Previously, the Docker daemon and ECS Agent would start up on host machine at boot, and our host service would have to race to initialize before the host became marked as ready to accept new tasks. Now, we start the Docker daemon and the ECS Agent only when we're ready to process Docker events, and we guarantee that the host service never misses the startup of a container. Furthermore, by waiting for the authentication handshake with the webserver to complete before starting the ECS agent, we ensure that a host that failed to authenticate (and therefore fails to deliver heartbeats to the Fractal webserver, thus getting killed by the webserver in a few minutes) does not accept any tasks.

## Publishing

The Fractal AMIs get automatically published to AWS EC2 through the `fractal-publish-ami.yml` GitHub Actions workflow. See `.github/workflows/fractal-publish-ami.yml` for the exact list of AWS regions supported and the AMI parameters.
The workflow uses [Packer](https://www.packer.io/) to automatically build and provision AMIs, then copy to all active regions. 

Packer is run with a single command and a configuration file (`ami_config.json`), which the workflow modifies to contain the parameters defined in the workflow. The first provisioner waits for the cloud instance to to boot, which seems to fix the ip connectivity issues that sporadically arise. The rest of the provisioners provide Packer with the setup scripts to run, when to reboot, how long to wait after reboot, etc. The post-processor creates a file `manifest.json` (upon successful build) that contains the created AMI IDs. If Packer fails, the file is not found and the GitHub Action fails.

