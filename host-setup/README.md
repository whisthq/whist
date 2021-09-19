# Fractal Host Setup

This subfolder contains the scripts to set up AWS EC2 instances for developing Fractal, and for setting up AWS EC2 AMIs (Amazon Machine Images) for our production EC2 instances running Fractal containers for our users.

An AMI is an operating system image, in our case a snapshot of Linux Ubuntu with specific packages, drivers and settings preinstalled and preconfigured, which can be easily loaded onto an EC2 instance instead of using default Linux Ubuntu. We use Fractal-specific AMIs with our EC2 instances as it results in faster deployment (the images are prebuilt) and ensures that all of our EC2 instances run the exact same operating system, which is specifically optimized for running Fractal containers.

The `setup_host.sh` script lets you set up an EC2 instance host either for development or deployment. You can pass `--help` or `--usage` into that script to see some more details.

## Setting Up a Development Instance

To set up your Fractal development instance:

- Create an Ubuntu Server 20.04 `g4dn.xlarge` EC2 instance on AWS region **us-east-1**, with at least 32 GB of storage (else you will run out of storage for the Fractal protocol and base container image).

  - Note that the 32 GB of persistent, EBS storage should be in addition to the built-in 125 GB of ephemeral storage! The ephemeral storage will not persist across reboots, so at this moment we do not use it for anything.
  - Note that the EC2 instance type must be **g4** or **g3** for GPU compatibility with our containers and streaming technology. We use g4 instances in because they have better performance and cost for our purposes.

- Add your EC2 instance to the security group **container-tester**, to enable proper networking rules. If you decide to set up your EC2 instance in a different AWS region, you will need to add it to the appropriate security group for that region, which may vary per region.

- Name your instance by making a new tag with key `Name` and value the desired name. (We now tag instances because we used to have all sorts of instances burning money for no reason, so we name all instances.) If an instance is unnamed, it is liable to be terminated! Your instance's name tag should include your name as well, so we can identify whose it is.

- Create a new keypair and save the `.pem` file as it is required to SSH into the instance, unless you use AWS Session Manager (AWS' version of SSH, accessible from the AWS console). Then, launch the instance.

- Set the keypair permissions to owner-readonly by running `chmod 400 your-keypair.pem`.

- If you use GitHub with SSH, set up a new SSH key and add it to Github ([Github instructions](https://docs.github.com/en/github/authenticating-to-github/connecting-to-github-with-ssh))

- Set up AWS credentials on your dev machine with `aws configure`. Enter your AWS credentials for the access key and secret key; for the region, use **us-east-1**.

- Then, run the following commands (adding the `-o` flag to shell scripts if you want to see output, see the README for each individual repository):

```bash
# clones `dev` by default
git clone git@github.com:fractal/fractal.git # via SSH
git clone https://github.com/fractal/fractal.git # via HTTPS

# set up the EC2 host for development
cd ~/fractal/host-setup
./setup_host.sh --localdevelopment

# build the Fractal protocol server
cd ~/fractal/protocol/
./build_protocol_targets.sh FractalServer

# build the Fractal base container image
cd ~/fractal/mandelboxes
./build_mandelbox_image.sh base

# build the Fractal Host Service
cd ~/fractal/host-service
make run # keep this open in a separate terminal

# run the Fractal base container image
cd ~/fractal/mandelboxes
./run_local_mandelbox_image.sh base
```

⚠️ If `./setup_host.sh` fails with the error `Unable to locate credentials`, run `aws configure` and then rerun the script. Enter your AWS credentials for the access key and secret key; for the region, use **us-east-1**.

⚠️ If the `./build_mandelbox_image.sh base` command fails due to apt being unable to fetch some archives (e.g. error: `E: Unable to fetch some archives, maybe run apt-get update or try with --fix-missing?`), call `docker system prune -af` first, then run `./build_mandelbox_image.sh base` again.

- Start a Fractal protocol client to connect to the Fractal protocol server running on your instance by following the instructions in [`protocol/client/README.md`](https://github.com/fractal/fractal/blob/dev/protocol/client/README.md). If a window pops up that streams the Fractal base application, which is currently **xterm**, then you are all set!

- Note that we shut down our dev instances when we're not using them, e.g. evenings and weekends. [Here](https://tryfractal.slack.com/archives/CPV6JFG67/p1611603277006600) are some helpful scripts to do so.

## Setting Up an AMI

To create an AMI:

- Create an Ubuntu Server 20.04 `g4dn.xlarge` EC2 instance on AWS region **us-east-1**, with at least 32 GB of persistent, EBS storage in addition to the 125 GB of ephemeral storage.

- Add your EC2 instance to the relevant production-ready security group(s) for the region you created it in. You can see which security group(s) currently-running production EC2 instances are attached to in the AWS console.

- Create a new keypair and save the `.pem` file as it is required to SSH into the instance, unless you use AWS Session Manager (AWS' version of SSH, accessible from the AWS console). Then, launch the instance.

- SSH/SSM into your instance and run the following commands:

```bash
# This clones `dev` by default --- checkout a different branch if needed
git clone https://github.com/fractal/fractal.git

# Set up the EC2 host with proper packages, drivers, and configs.
# To see the necessary values of ARGS, see the comment at the top of
# `deployment_setup_steps` in `setup_host.sh`.
cd ~/fractal/host-setup
./setup_host.sh --deployment [ARGS...]

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
