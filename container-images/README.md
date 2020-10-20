# Fractal Container Images

| Dockerfiles Building
|:--:|
|![Docker Image CI](https://github.com/fractalcomputers/container-images/workflows/Docker%20Image%20CI/badge.svg)|

This repository contains the code for containerizing the various applications that Fractal streams. The base image running the containerized Fractal protocol is under the `/base/` subfolder, and is used as a starter images for the application Dockerfiles which are in each of their respective application-type subfolders. This base image runs **Ubuntu 20.04** and installs everything needed to interface with the drivers and the Fractal protocol.

**Supported Applications**
- Google Chrome
- Mozilla Firefox
- Blender
- Blockbench
- Slack

See [Adding New Applications](#Adding-New-Applications).


## Development

To contribute to enhancing the general container images Fractal uses, you should contribute to the base Dockerfile.20 under `/base/`, unless your changes are application-specific, in which case you should contribute to the relevant Dockerfile.20 for the application in question. We strive to make container images as lean as possible to optimize for concurrency and reduce the realm of security attacks possible. Contributions should be made via pull requests to the `dev` branch, which is then merged up to `master`. The `master` branch gets automatically deployed to production by building and uploaded AWS ECR via GitHub Actions, and must not be pushed to unless thorough testing has been performed.

### Getting Started

When git cloning, ensure that all git submodules are pulled as follows:

```
git clone --recurse-submodules --branch $your-container-images-branch https://github.com/fractalcomputers/container-images ~/container-images
cd ~/container-images
```

Or, if you have sshkeys:

```
git clone --recurse-submodules --branch $your-container-images-branch git@github.com:fractalcomputers/container-images.git ~/container-images
```

Then, setup on your EC2 instance with the setup script from the [ECS Host Setup](https://github.com/fractalcomputers/ecs-host-setup/) repository: 

```
./setup_ubuntu20_host.sh
```

This will begin installing all dependencies and configurations required to run our container images on an AWS EC2 host. It will also ask if you want to connect your EC2 instance to an ECS cluster, which is optional for development. After the setup scripts run, you must `sudo reboot` for Docker to work properly. After rebooting, you may finally build the protocol and the base image by running:

```
./build_protocol.sh && ./build_container_image.sh base && ./run_local_container_image.sh base
```

### Building Images

To simply build the protocol in the `base/protocol` subrepository inside an Ubuntu 20.04 container, run:

```
./build_protocol.sh
```

To build a specific application's container image, run:

```
./build_container_image.sh APP
```

This takes a single argument, `APP`, which is the path to the target folder whose application container you wish to build. For example, the base container is built with `./build_container_image.sh base` and the chrome container is built with `./build_container_image.sh browsers/chrome`, since the relevant dockerfile is `browsers/chrome/Dockerfile.20`. This script names the built image as `fractal/$APP`, with a tag of `current-build`.

### Running Local Images

Once an image with tag `current-build` has been built locally via `build_container_images.sh`, it may be run locally by calling:

```
./run_local_container_image.sh APP [MOUNT]
```

As usual, `APP` is the path to the app folder. Meanwhile, `MOUNT` is an optional argument specifying whether to facilitate server protocol development by mounting and live-updating the `base/protocol` submodule. If `MOUNT=mount`, then the submodule is mounted; else, it is not. Note that this script should be used on EC2 instances as an Nvidia GPU is required for our containers and our protocol to function properly.

### Running Remote-Pushed Images

If an image has been pushed to ECR and you wish to test it, first ensure the AWS CLI is configured as in [Pushing Images](#pushing-images). Then, retrieve the tag you wish to run, either from ECR itself or by grabbing the relevant (full) Git commit hash from this repository, and run:

```
./run_remote_container_image.sh APP TAG [REGION] [MOUNT]
```

As above, `APP` is the path of the application you want to run, `REGION` optionally specifies the ECR region to pull from, with a default of `us-east-1`, and `MOUNT=mount` mounts the submodule. Here `TAG` is the full Git commit hash to run.

## Continous Integration & Publishing

We store our production container images on AWS Elastic Container Registry (ECR) and deploy them on AWS Elastic Container Service (ECS). 

### Manual Publishing

Assuming you have the AWS CLI installed and configured (automatic if you have run `./setup_ubuntu20_host.sh` and you are running on an EC2 instance), you may manually push your container images to ECR so long as your IAM role allows you to do this. On an EC2 instance, simply use the `ecr_manager_instance_profile`. Otherwise, your own AWS user account should have the needed privileges.

Once an image has been built via `./build_container_image.sh APP` and therefore taggedd with `current-build`, that image may be pushed by running:

```
./push_container_image.sh APP [REGION]
```

Here, `APP` is again the path to the relevant app folder; e.g., `base` or `browsers/chrome`. Meanwhile, `REGION` is an optional parameter specifying the AWS region to which to push. This defaults to `us-east-1`. The image is tagged with the full git commit hash of the current branch. Please manually push images sparingly, making sure to commit all of your changes before building and pushing.

### Continuous Integration




Currently, at every PR to `master` or `dev`, the Dockerfiles specified in `dockerfiles-building-ubuntu20.yml` will be built on GitHub Actions and status checks will be reported. These 


For every new application that you add support for, you should 




## Adding New Applications




Then, you need to add your new application to the continous integration system:

- Add the path to your new Dockerfile.20 in `.pre-commit-config.yaml`
- Add the path to your new Dockerfile.20 under `apps` in `push-images.yml`

If you're adding a new AWS region
- Add the region name under `aws-regions` in `push-images.yml`



## Styling

We use [Hadolint](https://github.com/hadolint/hadolint) to format the Dockerfiles in this project. Your first need to install Hadolint via your local package manager, i.e. `brew install hadolint`, and have the Docker daemon running before linting a specific file by running `hadolint <file-path>`.

We also have [pre-commit hooks](https://pre-commit.com/) with Hadolint support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` in the project folder, to instantiate the hooks for Hadolint. Dockerfile improvements will be printed to the terminal for all Dockerfiles specified under `args` in `.pre-commit-config.yaml`. If you need/want to add other Dockerfiles, you need to specify them there. If you have issues with Hadolint tracking non-Docker files, you can commit with `git commit -m [MESSAGE] --no-verify` to skip the pre-commit hook for files you don't want to track.
