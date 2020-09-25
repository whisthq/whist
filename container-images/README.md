# Fractal Container Images

| Base Ubuntu 18.04 | Base Ubuntu 20.04
|:--:|:--:|
|![Docker Image CI](https://github.com/fractalcomputers/container-images/workflows/Docker%20Image%20CI/badge.svg)|![Docker Image CI](https://github.com/fractalcomputers/container-images/workflows/Docker%20Image%20CI/badge.svg)|

This repository contains the Docker images containerizing the various applications that Fractal streams or is planning to stream from containers via our streaming technology. The base image running the containerized Fractal protocol is under the `/base/` subfolder, and is used as a starter image for the application Dockerfiles which are in each of their respective application-type subfolders.

**Supported Applications**

- None yet

## Usage

When git cloning, ensure that all git submodules are pulled as follows:

```
git clone --recurse-submodules --branch $your-container-images-branch https://github.com/fractalcomputers/container-images ~/container-images
cd ~/container-images
```

Or, if you have sshkeys,

```
git clone --recurse-submodules --branch $your-container-images-branch git@github.com:fractalcomputers/container-images.git ~/container-images
```

Then, setup on your EC2 instance with the setup scripts:

```
./setup.sh
```

Which will begin installing all dependencies and configuration. It will also ask if you want to connect your EC2 instance to an ECS cluster, which you may respond yes or no to. After the setup scripts run, you must `sudo reboot` for docker to work currently. After rebooting, you may finally run

```
./build.sh base 18 && ./run.sh base 18
```

## Development

To contribute to enhancing the general container images Fractal uses, you should contribute to the base Dockerfiles under `/base/`, unless your fixes are application-specific, in which case you should contribute to the relevant Dockerfile for the application. We strive to make container images as lean as possible to optimize for concurrency and reduce the realm of possible security attacks possible. Contributions should be made via pull requests to the `dev` branch, which is then merged up to `master` once deployed to production.

First, initialize the submodules with `git submodule update --init --recursive`. Then, you can build the container image with the `build.sh` script. It takes in a few parameters, `APP`, which determines the name of the folder/app to build, and `VERSION==18|20` which specificies the Ubuntu version.

```
# Usage
build.sh APP VERSION

# Example
build.sh base 18
```
After calling `build.sh`, you can publish to ECR (after having set up the `aws` CLI beforehand) using `push.sh APP VERSION`, or else run the container locally using `run.sh APP VERSION [MOUNT?]`. If the optional third argument is the word `mount`, then the container will mount the subrepository, so you can dev on the protocol without rebuilding the container each time.

For more details on how to build and develop the base container image, see `/base/README.md`. 

We have basic continuous integration set for each container image through GitHub Actions. At every PR to `master` or `dev`, the Docker images will be built to ensure they work, and the status badges are listed in the respective subfolders' READMEs. You should make sure that your code passes all tests under the Actions tab, and that you add to the continuous integration if you add support for new applications and generate new Dockerfiles.

## Styling

We use [Hadolint](https://github.com/hadolint/hadolint) to format the Dockerfiles in this project. Your first need to install Hadolint via your local package manager, i.e. `brew install hadolint`, and have the Docker daemon running before linting a specific file by running `hadolint <file-path>`. 

We also have [pre-commit hooks](https://pre-commit.com/) with Hadolint support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` to instantiate the hooks for Hadolint.Dockerfile improvements will be printed to the terminal for all Dockerfiles specified under `args` in `.pre-commit-config.yaml`. If you need/want to add other Dockerfiles, you need to specify them there.

## Publishing

We store our production container images on AWS Elastic Container Registry (ECR) and deploy them on AWS Elastic Container Service (ECS). To push to the AWS container registry, a `container-admin` AWS IAM user has been created with the following credentials:

- Access Key ID: `AKIA24A776SSFXQ6HLAK`
- Secret Key: `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`
- Console login link: `https://747391415460.signin.aws.amazon.com/console`
- Console password: `qxQ!McAhFu0)`

To publish, you first need to tag your Dockerfile before logging in and pushing the image to the repository:

```
# Tag
docker tag base-systemd-18:latest .dkr.ecr.us-east-2.amazonaws.com/fractal-containers:latest

# Login
aws ecr get-login-password  --region us-east-2 | docker login --username AWS --password-stdin 747391415460.dkr.ecr.us-east-2.amazonaws.com

# Push
docker push 747391415460.dkr.ecr.us-east-2.amazonaws.com/fractal-containers:latest
```
