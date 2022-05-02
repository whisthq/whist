# Whist Host Setup

This subfolder contains the scripts to set up AWS EC2 instances for developing Whist, and for setting up AWS EC2 AMIs (Amazon Machine Images) for our production EC2 instances running Whist containers (known as mandelboxes) for our users.

An AMI is an operating system image, in our case a snapshot of Linux Ubuntu with specific packages, drivers and settings preinstalled and preconfigured, which can be easily loaded onto an EC2 instance instead of using default Linux Ubuntu. We use Whist-specific AMIs with our EC2 instances as it results in faster deployment (the images are prebuilt) and ensures that all of our EC2 instances run the exact same operating system, which is specifically optimized for running Whist containers.

The `setup_host.sh` script lets you set up an EC2 instance host either for development or deployment. You can pass `--help` or `--usage` into that script to see some more details.

This folder also contains our configuration files for Filebeat, which we use for sending logs to Logz.io, our ELK platform, and for Docker, which we use to enforce strict container security.

## Setting Up a Personal Development Instance

To set up your personal Whist development instance:

- Create an Ubuntu Server 20.04 `g4dn.xlarge` EC2 instance on the AWS region of your choosing (the geographically closest to you), with at least 32 GB of storage (else you will run out of storage for the Whist protocol and base container image).

  - Note that the 32 GB of persistent, EBS storage should be in addition to the built-in 125 GB of ephemeral storage! The ephemeral storage will not persist across reboots, so at this moment we do not use it for anything on personal development instances.

  - Note that the EC2 instance type must be **g4** for GPU compatibility with our containers and streaming technology. We use these instances as they have the best GPU compatibility and performance, and come with fast-access ephemeral storage.

- Add your EC2 instance to the **MainVPCdev** VPC and security group **MandelboxesSecurityGroupDev**, to enable proper networking rules. We use Terraform to consolidate our AWS infrastructure, so the security group name should be the same across all regions. If you encounter some issues, contact the CODEOWNER of `backend/infrastructure/`.

- Name your instance by making a new tag with key `Name` and value the desired name. We tag instances because we used to have all sorts of instances burning money for no reason, so we name all instances. If an instance is unnamed, it is liable to be terminated! Your instance's name tag should include your name as well, so we can identify whose it is.

- Create a new keypair and save the `.pem` file as it is required to SSH into the instance, unless you use AWS Session Manager (AWS' version of SSH, accessible from the AWS console). Then, launch the instance.

- Set the keypair permissions to owner-readonly by running `chmod 400 your-keypair.pem` and connect to your `ssh -i "your-keypair.pem" ubuntu@ec2-[your-instance-public-ipv4-with-hyphens].compute-1.amazonaws.com`.

  - If you want to make connecting to your AWS instance easier, you can export an alias for the command in your `.zshrc`/`.bashrc`, depending on which shell you use, by adding `alias [your-alias]="ssh -i "[path-to-your-keypair].pem" ubuntu@ec2-[your-instance-public-ipv4].compute-1.amazonaws.com"`.

  - Alternatively, put the following in your .ssh/config file:

        Host foo
            Hostname        ec2-[your-instance-ip].compute-1.amazonaws.com
            User            ubuntu
            IdentityFile    [path-to-your-keypair].pem

    and then you can both `ssh foo` and `scp bar foo:~`.

- Run `sudo apt-get update` to refresh the packager manager, and run `sudo apt-get install awscli git`. **Note that all steps in below need to be run in the ubuntu userspace**.

- If you use GitHub with SSH, set up a new SSH key and add it to Github ([GitHub instructions](https://docs.github.com/en/github/authenticating-to-github/connecting-to-github-with-ssh))

- Set up AWS credentials on your development machine with `aws configure`. Enter your AWS credentials for the access key and secret key; for the region, use the region where your instance lives.

- Then, run the following commands (adding the `-o` flag to shell scripts if you want to see output -- see the README for each individual repository):

```bash
# clones `dev` by default
git clone git@github.com:whisthq/whist.git # via SSH, highly preferable
git clone https://github.com/whisthq/whist.git # via HTTPS, type password on every push

# set up the EC2 host for development
cd ~/whist/host-setup
./setup_host.sh --localdevelopment

# before moving to the next step, make sure to reboot as prompted
sudo reboot

# build the Whist base container image (swap base to browsers/chrome to build the Whist Chrome container)
cd ~/whist/mandelboxes
./build.sh base

# build and run the Whist Host Service
cd ~/whist/backend/services
make run_host_service # keep this open in a separate terminal

# run the Whist Chrome container image (swap base to browsers/brave to run the Whist Brave container)
# Note: this will give you a root shell inside the container; when the shell exits, the container will close as well
cd ~/whist/mandelboxes
./run.sh browsers/chrome
```

### Common Errors & Tips

⚠️ If `./setup_host.sh` fails with the error `Unable to locate credentials`, run `aws configure` and then rerun the script. Enter your AWS credentials for the access key and secret key; for the region, use the one where your instance lives.

⚠️ If the `./build.sh base` command fails due to apt being unable to fetch some archives (e.g. error: `E: Unable to fetch some archives, maybe run apt-get update or try with --fix-missing?`), call `docker system prune -af` first, then run `./build.sh base` again.

⚠️ If the `./run.sh foo` command doesn't appear to do anything, check the host-service output. If it complains about fetching objects, make sure that the build step for foo was successful.

- Start a Whist protocol client to connect to the Whist protocol server running on your instance by following the instructions in [`protocol/client/README.md`](https://github.com/whisthq/whist/blob/dev/protocol/client/README.md). If a window pops up that streams the selected application (Chrome, Brave, etc.), then you are all set!

- Note that we shut down our development instances when we're not using them, e.g. evenings and weekends. [Here](https://whisthq.slack.com/archives/CPV6JFG67/p1611603277006600) are some helpful scripts to do so.

### Accessing Logs

- Once the Docker container opens, if you would like to view Whist's server logs, run `tail -f /var/log/whist/protocol-out.log` (for stdout) and `tail -f /var/log/whist/protocol-err.log` (for stderr).

## Setting Up an AMI

We create AMIs for deployment programmatically via our deploy pipeline in GitHub Actions. You should **NEVER** create a production AMI manually! If you wish to create an AMI for yourself/for internal development, you can do so on the AWS console by right-clicking your EC2 instance and selecting `Create Image`.

If you have created an AMI in a specific AWS region (i.e. `us-east-1`) which you would like to replicate in a different AWS region (i.e. `us-west-1`), you can copy over your AMI. For complete details on how to copy over AMIs, see our [Documentation on Notion](https://www.notion.so/whisthq/4d91593ea0e0438b8bdb14c25c219d55?v=0c3983cf062d4c3d96ac2a65eb31761b&p=ca4fdec782894072a6dd63f32b494e1d).

## Deployment

The Whist AMIs get automatically published to AWS EC2 through the `whist-build-and-deploy.yml` GitHub Actions workflow. Please refer to that workflow for the exact list of AWS regions supported and the AMI parameters.
The workflow uses [Packer](https://www.packer.io/) to automatically build and provision AMIs, before copying to all active regions. Packer is run with a single command, a configuration file (`ami_config.json.pkr.hcl`), which is your source of truth for troubleshooting Packer issues.
