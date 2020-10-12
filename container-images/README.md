# Fractal Container Images

| Base Ubuntu 20.04
|:--:|
|![Docker Image CI](https://github.com/fractalcomputers/container-images/workflows/Docker%20Image%20CI/badge.svg)|

This repository contains the Docker images containerizing the various applications that Fractal streams or is planning to stream from containers via our streaming technology. The base images running the containerized Fractal protocol are under the `/base/` subfolder, and are used as a starter images for the application Dockerfiles which are in each of their respective application-type subfolders.

**Supported Applications**

-   None yet

## More detailed Usage

When git cloning, ensure that all git submodules are pulled as follows:

```
git clone --recurse-submodules --branch $your-container-images-branch https://github.com/fractalcomputers/container-images ~/container-images
cd ~/container-images
```

Or, if you have sshkeys,

```
git clone --recurse-submodules --branch $your-container-images-branch git@github.com:fractalcomputers/container-images.git ~/container-images
```

Then, setup on your EC2 instance with the setup script from the [ecs-host-setup repo](https://github.com/fractalcomputers/ecs-host-setup/):

```
./setup_ubuntu20_host.sh
```

Which will begin installing all dependencies and configuration. It will also ask if you want to connect your EC2 instance to an ECS cluster, which you may respond yes or no to. After the setup scripts run, you must `sudo reboot` for docker to work currently. After rebooting, you may finally run

```
./build_protocol.sh && ./build_container_image.sh base && ./run_local_container_image.sh base
```

### Building Images

To simply build the protocol in the `base/protocol` subrepository inside an Ubuntu 20 container, call

```
./build_protocol.sh
```

To build a container image, call

```
./build_container_image.sh APP
```

This takes a single argument, `APP`, which is the path to the target folder whose app container you wish to build. For example, the base container is built with `./build_container_image.sh base` and the chrome container is built with `./build_container_image.sh browsers/chrome`, since the relevant dockerfile is `browsers/chrome/Dockerfile.20`. This scripts names the built image as `fractal/$APP`, with a tag of `current-build`.

### Pushing Images

Assuming you have the AWS CLI installed and configured (automatic if you have run `./setup_ubuntu20_host.sh` and you are running on an EC2 instance), you may manually push your container images to ECR so long as your IAM role allows you to do this. On an EC2 instance, simply use the `ecr_manager_instance_profile`. Otherwise, your own AWS user account likely has the needed privileges. Else, to push to the AWS container registry, a `container-admin` AWS IAM user has been created with the following credentials:

-   Access Key ID: `AKIA24A776SSFXQ6HLAK`
-   Secret Key: `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`
-   Console login link: `https://747391415460.signin.aws.amazon.com/console`
-   Console password: `qxQ!McAhFu0)`

In any case, once an image has been built via `./build_container_image.sh APP` and therefore taggedd with `current-build`, that image may be pushed by running

```
./push_container_image.sh APP [REGION]
```

Here, `APP` is again the path to the relevant app folder; e.g., `base` or `browsers/chrome`. Meanwhile, `REGION` is an optional parameter specifying the AWS region to which to push. This defaults to `us-east-1`. The image is tagged with the full git commit hash of the current branch. Please manually push images sparingly, making sure to commit all of your changes before building and pushing.

Our current continuous integration setup builds and pushes using the above scripts either on manual workflow run or on push to the `master` branch.

### Running Local Images

Once an image with tag `current-build` has been built locally via `build_container_images.sh`, it may be run locally by calling

```
./run_local_container_image.sh APP [MOUNT]
```

As usual, `APP` is the path to the app folder. Meanwhile, `MOUNT` is an optional argument specifying whether to facilitate server protocol development by mounting and live-updating the `base/protocol` submodule. If `MOUNT=mount`, then the submodule is mounted; else, it is not.

### Running Remote Images

If an image has been pushed to ECR and you wish to test, first ensure the AWS CLI is configured as in [Pushing Images](#pushing-images). Retrieve the tag you wish to run, either from ECR itself, or by grabbing the relevant (full) git commit hash from this repository. Then, run

```
./run_remote_container_image.sh APP TAG [REGION] [MOUNT]
```

As above, `APP` is self-explanatory, `REGION` optionally specifies the AWS region, with a default of `us-east-1`, and `MOUNT=mount` mounts the submodule. Here `TAG` is the full git commit hash to run.

## Development

To contribute to enhancing the general container images Fractal uses, you should contribute to the base Dockerfiles under `/base/`, unless your fixes are application-specific, in which case you should contribute to the relevant Dockerfile for the application. We strive to make container images as lean as possible to optimize for concurrency and reduce the realm of possible security attacks possible. Contributions should be made via pull requests to the `dev` branch, which is then merged up to `master` once deployed to production. The `master` branch is going to automatically deploy to production via AWS ECR (this is a current TODO), and must not be pushed to unless thorough testing has been performed.

For general development, refer to the `/base` subfolder README, and to your specific applications' subfolder README for application-specific development. See below for our continous integration process.

## Styling

We use [Hadolint](https://github.com/hadolint/hadolint) to format the Dockerfiles in this project. Your first need to install Hadolint via your local package manager, i.e. `brew install hadolint`, and have the Docker daemon running before linting a specific file by running `hadolint <file-path>`.

We also have [pre-commit hooks](https://pre-commit.com/) with Hadolint support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` to instantiate the hooks for Hadolint.Dockerfile improvements will be printed to the terminal for all Dockerfiles specified under `args` in `.pre-commit-config.yaml`. If you need/want to add other Dockerfiles, you need to specify them there. If you have issues with Hadolint tracking non-Docker files, you can commit with `git commit -m [MESSAGE] --no-verify` to skip the pre-commit hook.

## Continous Integration & Publishing

We store our production container images on AWS Elastic Container Registry (ECR) and deploy them on AWS Elastic Container Service (ECS). We are currently building a true continuous delivery process, where at every PR to `master`, the images get built by GitHub Actions and then pushed to ECR automatically if all builds and tests pass, at which point the webserver will automatically pick new images for production deployment.

Currently, at every PR to `master` or `dev`, the Docker base images will be built on GitHub Actions and status checks will be reported. For every new application that you add support for, you should create a new `docker-[YOUR APP]-ubuntu[18||20].yml` file to test that it properly build and then uploads to ECR automatically. You can follow the format of `docker-base-ubuntu20.yml` as needed.

### Pushing to ECR manually (this is outdated; we have a script documented above for this)

To push to the AWS container registry, a `container-admin` AWS IAM user has been created with the following credentials:

-   Access Key ID: `AKIA24A776SSFXQ6HLAK`
-   Secret Key: `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`
-   Console login link: `https://747391415460.signin.aws.amazon.com/console`
-   Console password: `qxQ!McAhFu0)`

To publish, you first need to tag your Dockerfile before logging in and pushing the image to the repository:

```
# Tag
(NOTE [savvy]: This seems out of date, even after replacing 18 with 20? I thought we used commits, not tags? TODO)
docker tag base-systemd-20:latest .dkr.ecr.us-east-2.amazonaws.com/fractal-containers:latest

# Login
aws ecr get-login-password  --region us-east-2 | docker login --username AWS --password-stdin 747391415460.dkr.ecr.us-east-2.amazonaws.com

# Push
docker push 747391415460.dkr.ecr.us-east-2.amazonaws.com/fractal-containers:latest
```
