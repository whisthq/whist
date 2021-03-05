# Fractal Container Images

This repository contains the code for containerizing the various applications that Fractal streams. The base Dockerfile.20 running the containerized Fractal protocol is under the `/base/` subfolder, and is used as a starter image for the application Dockerfiles which are in each of their respective application-type subfolders. This base image runs **Ubuntu 20.04** and installs everything needed to interface with the drivers and the Fractal protocol.

TODO: add a directory structure like Nick did for the protocol, which is really good. (https://github.com/fractal/fractal/issues/1133)

### Supported Applications

All of the following applications are based off of the **Ubuntu 20.04 Base Image**.

| Browsers         | Creative (2D/3D) | Creative (Video) | Productivity |
| ---------------- | ---------------- | ---------------- | ------------ |
| Google Chrome    | Blender          | Lightworks       | Slack        |
| Mozilla Firefox  | Blockbench       | Kdenlive         | Notion       |
| Brave Browser    | Figma            |                  | Discord      |
| Sidekick Browser | TextureLab       |                  | R Studio     |
|                  | Gimp             |                  | FreeCAD      |
|                  | Inkscape         |                  |              |
|                  | Krita            |                  |              |

See [Adding New Applications](#Adding-New-Applications) for details on how to add support for new applications and integrate them with our continuous delivery pipeline.

## Development

To contribute to enhancing the general container images Fractal uses, you should contribute to the base Dockerfile.20 under `/base/`, unless your changes are application-specific, in which case you should contribute to the relevant Dockerfile.20 for the application in question. We strive to make container images as lean as possible to optimize for concurrency and reduce the realm of security attacks possible.

Contributions should be made via pull requests to the `dev` branch, which is then merged up to `master`. The `master` branch gets automatically deployed to production by building and uploading to [GHCR](https://ghcr.io) via GitHub Actions, and must not be pushed to unless thorough testing has been performed. Currently, at every PR to `master` or `dev`, the Dockerfiles specified in `dockerfiles-building-ubuntu20.yml` will be built on GitHub Actions and status checks will be reported. These tests need to be pass before merging is approved.

### Getting Started

When git cloning, ensure that all git submodules are pulled as follows:

```
git clone --recurse-submodules --branch $your-container-images-branch https://github.com/fractal/container-images ~/container-images
cd ~/container-images
```

Or, if you have sshkeys:

```
git clone --recurse-submodules --branch $your-container-images-branch git@github.com:fractal/container-images.git ~/container-images
```

Then, setup on your EC2 instance with the setup script from the [ECS Host Setup](https://github.com/fractal/ecs-host-setup/) repository:

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

This takes a single argument, `APP`, which is the path to the target folder whose application container you wish to build. For example, the base container is built with `./build_container_image.sh base` and the Chrome container is built with `./build_container_image.sh browsers/chrome`, since the relevant Dockerfile is `browsers/chrome/Dockerfile.20`. This script names the built image as `fractal/$APP`, with a tag of `current-build`.

You first need to build the protocol and then build the base image before you can finally build a specific application image.

**NOTE:** For production we do not cache builds of Dockerfiles. This is for two reasons:

1. Using build caches will almost surely lead to outdated versions of packages being present in the final images, which exposes publicly-known security flaws.
2. It is also more expensive to keep a builder machine alive 24/7 than to just build them on the fly.

### Running Local Images

Once an image with tag `current-build` has been built locally via `build_container_images.sh`, it may be run locally by calling:

```
[FRACTAL_DPI=96] ./run_local_container_image.sh APP [MOUNT]
```

As usual, `APP` is the path to the app folder. Meanwhile, `MOUNT` is an optional argument specifying whether to facilitate server protocol development by mounting and live-updating the `base/protocol` submodule. If `MOUNT=mount`, then the submodule is mounted; else, it is not. Note that this script should be used on EC2 instances as an Nvidia GPU is required for our containers and our protocol to function properly.

You can optionally override the default value of `96` for `FRACTAL_DPI` by setting the eponymous environment variable prior to running the container image. This might be useful if you are testing on a high-DPI screen.

### Running Remote-Pushed Images

If an image has been pushed to GHCR and you wish to test it, you first need to authenticate Docker to allow you to pull the relevant image. To do this, run the following:

```
echo <PAT> | docker login --username <GH_USERNAME> --password-stdin ghcr.io
```

Replace `<PAT>` with a [Github Personal Access Token](https://docs.github.com/en/free-pro-team@latest/github/authenticating-to-github/creating-a-personal-access-token) with at least the `package` scope. Also, replace `<GH_USERNAME>` with your GitHub username.

Then, retrieve the tag you wish to run by grabbing the relevant (full) Git commit hash from this repository, and run:

```
[FRACTAL_DPI=96] ./run_remote_container_image.sh APP TAG [MOUNT]
```

The argument `TAG` is the full Git commit hash to run. All other configuration is the same as for the local case.

### Connecting to Images

Before connecting to the server protocol that runs in the container, the host service needs to receive a `set_container_start_values` request in order to allow the container to run. For now, this request automatically made by `run_container_image.sh`. This request sets the DPI and User ID.

If you are using a high-DPI screen, you may want to pass in the optional DPI argument by setting the `FRACTAL_DPI` environment variable on your host. Additionally, if you want to save your configs between sessions, then set the `FRACTAL_USER_ID` environment variable on your host.

Currently, it is important to wait 5-10 seconds after making the cURL request before connecting to the container via `./FractalClient -w [width] -h [height] [ec2-ip-address]`. This is due to a race condition between the `fractal-audio.service` and the protocol audio capturing code: (See issue [#360](https://github.com/fractal/fractal/issues/360)).

## Publishing

We store our production container images on GitHub Container Registry (GHCR) and deploy them on AWS Elastic Container Service (ECS).

### Manual Publishing

Once an image has been built via `./build_container_image.sh APP` and therefore tagged with `current-build`, that image may be manually pushed to GHCR by running (note, however, this is usually done by the CI. You shouldn't have to do this except in very rare circumstances. If you do, make sure to commit all of your changes before building and pushing):

```
GH_PAT=xxx GH_USERNAME=xxx ./push_container_image.sh APP
```

Replace the environment variables `GH_PAT` and `GH_USERNAME` with your GitHub personal access token and username, respectively. Here, `APP` is again the path to the relevant app folder; e.g., `base` or `browsers/chrome`. The image is tagged with the full git commit hash of the current branch.

### Continous Delivery

This is how we push to production. For every push to `master`, all applications that have a Dockerfile get automatically built and pushed to all AWS regions specified under `aws-regions` in `.github/workflows/push-images.yml`. This will then automatically trigger a new release of all the ECS task definitions in `fractal/ecs-task-definitions`, which need to be updated in production to point to our new container image tags.

#### Adding New Applications

For every new application that you add support for, in addition to creating its own subfolder under the relevant category and creating application-specific **Dockerfile.20**, you need to:

-   Add the path to your new Dockerfile.20 in `.pre-commit-config.yaml`, for pre-commit hooks
-   Update the list of supported applications in this README

Note that before your new application is ready to go into production, you need to also edit the database with the app's logo, terms of service link, description, task definition link, etc.

And, if you're adding a new AWS region, you should add the region name under `aws-regions` in `push-images.yml`.

## Styling

We use [Hadolint](https://github.com/hadolint/hadolint) to format the Dockerfiles in this project. Your first need to install Hadolint via your local package manager, i.e. `brew install hadolint`, and have the Docker daemon running before linting a specific file by running `hadolint <file-path>`.

We also have [pre-commit hooks](https://pre-commit.com/) with Hadolint support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` in the project folder, to instantiate the hooks for Hadolint. Dockerfile improvements will be printed to the terminal for all Dockerfiles specified under `args` in `.pre-commit-config.yaml`. If you need/want to add other Dockerfiles, you need to specify them there. If you have issues with Hadolint tracking non-Docker files, you can commit with `git commit -m [MESSAGE] --no-verify` to skip the pre-commit hook for files you don't want to track.
