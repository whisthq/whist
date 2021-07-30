# Fractal Host Setup

This subfolder contains the scripts to set up AWS EC2 instances for developing Fractal, and for setting up AWS EC2 AMIs (Amazon Machine Images) for our production EC2 instances running Fractal containers for our users.

An AMI is an operating system image, in our case a snapshot of Linux Ubuntu with specific packages, drivers and settings preinstalled and preconfigured, which can be easily loaded onto an EC2 instance instead of using default Linux Ubuntu. We use Fractal-specific AMIs with our EC2 instances as it results in faster deployment (the images are prebuilt) and ensures that all of our EC2 instances run the exact same operating system, which is specifically optimized for running Fractal containers.

The `setup_ubuntu20_host.sh` script lets you set up a general EC2 instance host for development (by setting up proper NVIDIA drivers and Docker daemon configs and filters), while running `setup_ubuntu20_host.sh` followed by `setup_ubuntu20_ami_host.sh` sets up an EC2 host that can be manually stored as an AMI for programmatic deployment.

## Setting Up a Development Instance

To set up your Fractal development instance:

- Create an Ubuntu Server 20.04 `g4dn.xlarge` EC2 instance on AWS region **us-east-1**, with at least 32 GB of storage (else you will run out of storage for the Fractal protocol and base container image).

  - Note that the 32 GB of persistent, EBS storage should be in addition to the built-in 125 GB of ephemeral storage! The ephemeral storage will not persist across reboots, so at this moment we do not use it for anything.
  - Note that the EC2 instance type must be **g4** or **g3** for GPU compatibility with our containers and streaming technology. We use g4 instances in because they have better performance and cost for our purposes.

- Add your EC2 instance to the security group **container-tester**, to enable proper networking rules. If you decide to set up your EC2 instance in a different AWS region, you will need to add it to the appropriate security group for that region, which may vary per region.

- Name your instance by making a new tag with key `Name` and value the desired name. (We now tag instances because we used to have all sorts of instances burning money for no reason, so we name all instances.) If an instance is unnamed, it is liable to be terminated!

- Create a new keypair and save the `.pem` file as it is required to SSH into the instance, unless you use AWS Session Manager (AWS' version of SSH, accessible from the AWS console). Then, launch the instance.

- Set the keypair permissions to owner-readonly by running `chmod 400 your-keypair.pem`.

- SSH/SSM into your instance and install the latest stable version of `Go` via the following ([instructions](https://linuxize.com/post/how-to-install-go-on-ubuntu-20-04/)).

- If you use Github with SSH, set up a new SSH key and add it to Github ([Github instructions](https://docs.github.com/en/github/authenticating-to-github/connecting-to-github-with-ssh))

- Then, run the following commands (adding the `-o` flag to shell scripts if you want to see output, see the README for each individual repository):

```
# clones `dev` by default
git clone git@github.com:fractal/fractal.git # via SSH
git clone https://github.com/fractal/fractal.git # via HTTPS

# set up the EC2 host for development
cd ~/fractal/host-setup
./setup_localdev_dependencies.sh
sudo reboot

# build the Fractal protocol server
cd ~/fractal/protocol/
./build_server_protocol.sh

# build the Fractal base container image
cd ~/fractal/container-images
./build_container_image.sh base

# build the Fractal Host Service
cd ~/fractal/host-service
make run # keep this open in a separate terminal

# run the Fractal base container image
cd ~/fractal/container-images
./run_local_container_image.sh base
```

- If `./setup_ubuntu20_host.sh` fails with the error `Unable to locate credentials`, run `aws configure` and then rerun the script. Enter your AWS credentials for the access key and secret key; for the region, use **us-east-1**.

- Start a Fractal protocol client to connect to the Fractal protocol server running on your instance by following the instructions in [`protocol/client/README.md`](https://github.com/fractal/fractal/blob/dev/protocol/client/README.md). If a window pops up that streams the Fractal base application, which is currently **xterm**, then you are all set!

- Note that we shut down our dev instances when we're not using them, e.g. evenings and weekends. [Here](https://tryfractal.slack.com/archives/CPV6JFG67/p1611603277006600) are some helpful scripts to do so.

## Setting Up an AMI

To create an AMI:

- Create an Ubuntu Server 20.04 `g4dn.xlarge` EC2 instance on AWS region **us-east-1**, with at least 32 GB of persistent, EBS storage in addition to the 125 GB of ephemeral storage.

- Add your EC2 instance to the relevant production-ready security group(s) for the region you created it in. You can see which security group(s) currently-running production EC2 instances are attached to in the AWS console.

- Create a new keypair and save the `.pem` file as it is required to SSH into the instance, unless you use AWS Session Manager (AWS' version of SSH, accessible from the AWS console). Then, launch the instance.

- SSH/SSM into your instance and run the following commands:

```
# clones `dev` by default
git clone https://github.com/fractal/fractal.git

# set up the EC2 host with proper packages and drivers,
# and sets Fractal Docker daemon configs from docker-demon-config/daemon.json
cd ~/fractal/host-setup
./setup_ubuntu20_host.sh
sudo reboot

# set up networking rules and system configs and services
# this script also copies over the userdata-bootstrap.sh script
cd ~/fractal/host-setup
./setup_ubuntu20_ami_host.sh

# VERY IMPORTANT: remove all Fractal code!
cd ~
rm -rf fractal
```

- Manually test that this instance can be attached to an AWS EC2 instances cluster and that it can be connected to, and then save it as an AMI in the AWS console.

**NOTE**: If you want to see the actual userdata that gets passed into the EC2 hosts, it's in the subfolder `webserver` in the file [`app/helpers/utils/aws/base_userdata_template.sh`](https://github.com/fractal/fractal/blob/dev/webserver/app/helpers/utils/aws/base_userdata_template.sh).

## Copying AMIs Across AWS Regions

If you have created an AMI in a specific AWS region (i.e. `us-east-1`) which you would like to replicate in a different AWS region (i.e. `us-west-1`), you can either re-run the scripts in a different region and start the process from scratch, or you can copy over your AMI (which is much faster). For complete details on how to copy over AMIs, see our [Documentation on Notion](https://www.notion.so/tryfractal/4d91593ea0e0438b8bdb14c25c219d55?v=0c3983cf062d4c3d96ac2a65eb31761b&p=ca4fdec782894072a6dd63f32b494e1d).

## Publishing

The Fractal AMIs get automatically published to AWS EC2 through the `fractal-build-and-deploy.yml` GitHub Actions workflow. See that workflow for the exact list of AWS regions supported and the AMI parameters.
The workflow uses [Packer](https://www.packer.io/) to automatically build and provision AMIs, then copy to all active regions.

Packer is run with a single command, a configuration file (`ami_config.json.pkr.hcl`), and a variables file (`packer_vars.json`) that is generated by the workflow. The first provisioner waits for the cloud instance to to boot, which seems to fix the ip connectivity issues that sporadically arise. The rest of the provisioners provide Packer with the setup scripts to run, when to reboot, how long to wait after reboot, etc. The post-processor creates a file `manifest.json` (upon successful build) that contains the created AMI IDs. If Packer fails, the file is not found and the GitHub Action fails.
