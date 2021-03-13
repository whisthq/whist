# Fractal Container Images

This repository contains the code for containerizing the various applications that Fractal streams. The base Dockerfile.20 running the containerized Fractal protocol is under the `/base/` subfolder, and is used as a starter image for the application Dockerfiles which are in each of their respective application-type subfolders. This base image runs **Ubuntu 20.04** and installs everything needed to interface with the drivers and the Fractal protocol.

## File Structure

A tree structure is provided below:

```
./container-images
├── base
│   ├── Dockerfile.20 <- Base container image on which all of our application images depend
│   ├── config
│   │   ├── 01-fractal-nvidia.conf <- Configuration file for a Nvidia GPU-powered virtual display
│   │   ├── 02-fractal-dummy.conf <- Configuration file for a software-powered virtual display
│   │   ├── Xmodmap <- Configuration file for keystroke mappings on Linux Ubuntu
│   │   ├── app-config-map.json <- Map for applications settings configuration location on Ubuntu
│   │   ├── awesome-rc.lua <- Configuration file for our AwesomeWM window manager
│   │   ├── fractal-display-config.env <- Environment variables for the Fractal virtual displays
│   │   ├── gtk-3-settings.ini <- Configuration file for GTK-basedapplications
│   │   ├── pulse-client.conf <- Configuration file for our PulseAudio server
│   │   ├── qt4-settings.conf <- Configuration file for QT4-based applications
│   │   └── systemd-logind-override-ubuntu20.conf <- Custom configuration for hostnames
│   ├── scripts
│   │   ├── docker-entrypoint.sh <- First script run within a container, to start systemd
│   │   ├── entry.sh <- Script to start the `fractal` systemd user within a container
│   │   ├── run-fractal-server.sh <- Script to start the Fractal server protocol
│   │   ├── update-xorg-conf.sh <- Script to update the X Server to use the Nvidia GPU and uinput node
│   │   └── xinitrc <- Script to start the window manager and Fractal application
│   └── services
│       ├── fractal-audio.service <- Systemd service to start a PulseAudio server
│       ├── fractal-display.service <- Systemd service to start the Fractal virtual display
│       ├── fractal-entrypoint.service <- Systemd service to start the Fractal systemd user
│       ├── fractal-protocol.service <- Systemd service to start the Fractal server protocol
│       └── fractal-update-xorg-conf.service <- Systemd service to update the X Server to the Fractal configuration
├── browsers
│   ├── brave
│   │   ├── Dockerfile.20 <- Container image for Brave Browser
│   │   └── install-extensions.sh <- Helper script to install Chromium extensions onto Brave
│   ├── chrome
│   │   ├── Dockerfile.20 <- Container image for Google Chrome
│   │   ├── install-extensions.sh <- Helper script to install Chromium extensions onto Chrome
│   │   └── start-chrome.sh <- Helper script to start Chrome with specific flags
│   ├── firefox
│   │   └── Dockerfile.20 <- Container image for Mozilla Firefox
│   └── sidekick
│       └── Dockerfile.20 <- Container image for Sidekick Browser
├── build_container_image.sh <- Helper script to build a specific Docker image
├── creative
│   ├── blender
│   │   ├── Dockerfile.20 <- Container image for Blender
│   │   └── userpref.blend <- Fractal-provided default Blender user settings
│   ├── blockbench
│   │   └── Dockerfile.20 <- Container image for Blockbench
│   ├── figma
│   │   ├── Dockerfile.20 <- Container image for the Figma desktop application
│   │   ├── figma-app.desktop <- Desktop shortcut file associated with the Figma desktop application
│   │   ├── figma-font-bindings
│   │   │   ├── index.d.ts <- Module file for the Figma font bindings
│   │   │   ├── native
│   │   │   │   ├── Cargo.lock <- Rust Figma font bindings lock file
│   │   │   │   ├── Cargo.toml <- Rust Figma font bindings project file
│   │   │   │   ├── build.rs <- Main build file for generating fonts object
│   │   │   │   └── src
│   │   │   │       ├── async_font.rs <- Main file to generate font objects
│   │   │   │       └── lib.rs <- Helper functions to generate font objects
│   │   │   ├── package-lock.json <- Figma font bindings module lock file
│   │   │   └── package.json <- Figma font bindings module project file
│   │   └── run-figma.sh <- Helper script to start the Figma desktop application
│   ├── gimp
│   │   ├── Dockerfile.20 <- Container image for Gimp
│   │   └── gimprc <- Fractal Gimp configuration file
│   ├── godot
│   │   └── Dockerfile.20 <- Container image for Godot Game Engine
│   └── lightworks
│       └── Dockerfile.20 <- Container image for Lightworks Video Editor
├── helper-scripts
│   ├── build_container_image.py <- Helper script to build a/many Docker image(s)
│   ├── copy_protocol_build.sh <- Helper script to copy the compiled Fractal server protocol between folders
│   ├── find_images_in_git_repo.sh <- Helper script to find all Dockerfiles in this folder tree
│   └── run_container_image.sh <- Helper script to run a build container image
├── productivity
│   ├── notion
│   │   ├── Dockerfile.20 <- Container image for the Notion desktop application
│   │   ├── notion-app.desktop <- Desktop shortcut file associated with the Notion desktop application
│   │   └── run-notion.sh <- Helper script to start Notion with the right parameters
│   └── slack
│       ├── Dockerfile.20 <- Container image for Slack
│       └── run-slack.sh <- Helper script to start Slack with the right parameters
├── push_container_image.sh <- Helper script to push a built container image to GHCR
├── run_local_container_image.sh <- Helper script to run a locally-built container image
├── run_remote_container_image.sh <- Helper script to fetch and run a container image stored on GHCR
└── testing-scripts
    ├── connection_tester.py <- Helper script to test UDP/TCP connectivity between a Fractal protocol server in a container and a client
    ├── uinput_server.c <- Helper script to set up a Linux uinput server in a container
    └── uinput_tester.py <- Helper script to test sending uinput events to a uinput server in a container
```

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

## Styling

We use [Hadolint](https://github.com/hadolint/hadolint) to format the Dockerfiles in this project. Your first need to install Hadolint via your local package manager, i.e. `brew install hadolint`, and have the Docker daemon running before linting a specific file by running `hadolint <file-path>`.

We also have [pre-commit hooks](https://pre-commit.com/) with Hadolint support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` in the project folder, to instantiate the hooks for Hadolint. Dockerfile improvements will be printed to the terminal for all Dockerfiles specified under `args` in `.pre-commit-config.yaml`. If you need/want to add other Dockerfiles, you need to specify them there. If you have issues with Hadolint tracking non-Docker files, you can commit with `git commit -m [MESSAGE] --no-verify` to skip the pre-commit hook for files you don't want to track.
