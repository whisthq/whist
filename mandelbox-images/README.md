# Fractal Mandelbox Images

At Fractal, we use the term "mandelbox" to refer to what is essentially an application that we deliver, combined with the Fractal-specific customizations that make everything work (securely). At the moment, our mandelboxes run on Docker.

This repository contains the code for "mandelbox-izing" the various applications that Fractal streams. The base Dockerfile.20 running the Fractal protocol is under the `/base/` subfolder, and is used as a starter image for the application Dockerfiles which are in each of their respective application-type subfolders. This base image runs **Ubuntu 20.04** and installs everything needed to interface with the drivers and the Fractal protocol.

## File Structure

A tree structure is provided below:

```
./mandelbox-images
├── base
│   ├── Dockerfile.20 <- Base mandelbox image on which all of our application images depend
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
│   │   ├── docker-entrypoint.sh <- First script run within a mandelbox, to start systemd
│   │   ├── entry.sh <- Script to start the `fractal` systemd user within a mandelbox
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
│   ├── chrome
│   │   ├── Dockerfile.20 <- Mandelbox image for Google Chrome
│   │   ├── install-extensions.sh <- Helper script to install Chromium extensions onto Chrome
│   │   └── start-chrome.sh <- Helper script to start Chrome with specific flags
│   ├── firefox
│   │   └── Dockerfile.20 <- Mandelbox image for Mozilla Firefox
├── build_mandelbox_image.sh <- Helper script to build a specific mandelbox image
├── helper_scripts
│   ├── build_mandelbox_image.py <- Helper script to build a/many mandelbox image(s)
│   ├── copy_protocol_build.sh <- Helper script to copy the compiled Fractal server protocol between folders
│   ├── find_images_in_git_repo.sh <- Helper script to find all Dockerfiles in this folder tree
│   └── run_mandelbox_image.sh <- Helper script to run a mandelbox image
│   └── run_mandelbox_image.py <- Helper script to run a mandelbox image
├── push_mandelbox_image.sh <- Helper script to push a built mandelbox image to GHCR
├── run_local_mandelbox_image.sh <- Helper script to run a locally-built mandelbox image
├── run_remote_mandelbox_image.sh <- Helper script to fetch and run a mandelbox image stored on GHCR
└── testing_scripts
    ├── connection_tester.py <- Helper script to test UDP/TCP connectivity between a Fractal protocol server in a mandelbox and a client
    ├── uinput_server.c <- Helper script to set up a Linux uinput server in a mandelbox
    └── uinput_tester.py <- Helper script to test sending uinput events to a uinput server in a mandelbox
```

## Development

To contribute to enhancing all the mandelbox images Fractal uses, you should contribute to the base Dockerfile.20 under `/base/`, unless your changes are application-specific, in which case you should contribute to the relevant Dockerfile.20 for the application in question. We strive to make mandelbox images as lean as possible to optimize for concurrency and reduce the realm of security attacks possible.

### Getting Started

After cloning the repo, set up your EC2 instance with the setup script from the `host-setup` subrepo.:

```shell
./setup_localdev_dependencies.sh
```

This will begin installing all dependencies and configurations required to run our mandelbox images on an AWS EC2 host. It will also ask if you want to connect your EC2 instance to an ECS cluster, which is optional for development. After the setup scripts run, you must `sudo reboot` for Docker to work properly. After rebooting, you may finally build the protocol and the base image by running:

```shell
../protocol/build_server_protocol.sh && ./build_mandelbox_image.sh base && ./run_local_mandelbox_image.sh base
```

### Building Images

To build the server protocol for use in a mandelbox image (for example with the `--update-protocol` parameter to `run_mandelbox_image.sh`), run:

```shell
../protocol/build_server_protocol.sh
```

To build a specific application's mandelbox image, run:

```shell
./build_mandelbox_image.sh APP
```

This takes a single argument, `APP`, which is the path to the target folder whose application mandelbox you wish to build. For example, the base mandelbox is built with `./build_mandelbox_image.sh base` and the Chrome mandelbox is built with `./build_mandelbox_image.sh browsers/chrome`, since the relevant Dockerfile is `browsers/chrome/Dockerfile.20`. This script names the built image as `fractal/$APP`, with a tag of `current-build`.

You first need to build the protocol and then build the base image before you can finally build a specific application image.

**NOTE:** For production we do not cache builds of mandelboxes. This is for two reasons:

1. Using build caches will almost surely lead to outdated versions of packages being present in the final images, which exposes publicly-known security flaws.
2. It is also more expensive to keep a builder machine alive 24/7 than to just build them on the fly.

### Running Local Images

Before you can run mandelbox images (local or remote), make sure you have the host service running in a separate terminal with `cd ../host-service && make run`.

Once an image with tag `current-build` has been built locally via `build_mandelbox_images.sh`, it may be run locally by calling:

```
./run_local_mandelbox_image.sh APP [OPTIONS...]
```

As usual, `APP` is the path to the app folder. Note that this script should be used on EC2 instances as an Nvidia GPU is required for our mandelboxes and our protocol to function properly.

There are some other options available to control properties of the resulting mandelbox, like whether the server protocol should be replaced with the locally-built version. Run `./run_local_mandelbox_image.sh --help` to see all the other configuration options.

### Running Remote-Pushed Images

Before you can run mandelbox images (local or remote), make sure you have the host service running in a separate terminal with `cd ../host-service && make run`.

If an image has been pushed to GHCR and you wish to test it, you first need to authenticate Docker to allow you to pull the relevant image. To do this, run the following:

```
echo <PAT> | docker login --username <GH_USERNAME> --password-stdin ghcr.io
```

Replace `<PAT>` with a [GitHub Personal Access Token](https://docs.github.com/en/free-pro-team@latest/github/authenticating-to-github/creating-a-personal-access-token) with at least the `package` scope. Also, replace `<GH_USERNAME>` with your GitHub username.

Then, retrieve the tag you wish to run by grabbing the relevant (full) Git commit hash from this repository, and run:

```
./run_remote_mandelbox_image.sh APP_WITH_ENVIRONMENT TAG [OPTIONS...]
```

The argument `TAG` is the full Git commit hash to run. Note that `APP_WITH_ENVIRONMENT` is something like `dev/browsers/chrome`, for instance. All other configuration is the same as the local case, and the `--help` argument works with this script too.

### Connecting to Images

If you want to save your configs between sessions, then pass in a user ID and config encryption token. In case you don't want the server protocol to auto-shutdown after 60 seconds, you can set the timeout with another argument. As mentioned above, pass in `--help` to one of the mandelbox image-running scripts to see all the available options.

Currently, it is important to wait 5-10 seconds after making the cURL request before connecting to the mandelbox via `./FractalClient -w [width] -h [height] [ec2-ip-address]`. This is due to a race condition between the `fractal-audio.service` and the protocol audio capturing code: (See issue [#360](https://github.com/fractal/fractal/issues/360)).

## Publishing

We store our production mandelbox images on GitHub mandelbox Registry (GHCR) and deploy them on EC2 via our host service.

### Manual Publishing

Once an image has been built via `./build_mandelbox_image.sh APP` and therefore tagged with `current-build`, that image may be manually pushed to GHCR by running (note, however, this is usually done by the CI. You shouldn't have to do this except in very rare circumstances. If you do, make sure to commit all of your changes before building and pushing):

```
GH_PAT=xxx GH_USERNAME=xxx ./push_mandelbox_image.sh APP ENVIRONMENT
```

Replace the environment variables `GH_PAT` and `GH_USERNAME` with your GitHub personal access token and username, respectively. Here, `APP` is again the path to the relevant app folder; e.g., `base` or `browsers/chrome`. Environment is either `dev`, `staging`, `prod`, or nothing. The image is tagged with the full git commit hash of the current branch.

### Continuous Delivery

For every push to `dev`, `staging`, or `prod`, all applications that have a Dockerfile get automatically built and pushed to GHCR. These images are then loaded up into AMIs and deployed.

### Useful Debugging Practices

If `./build_mandelbox_images.sh` is failing, try running with `./build-mandelbox_images.sh -o` for logging output.

If the error messages seem to be related to fetching archives, try `docker system prune -af`. It's possible that Docker has cached out-of-date steps generated from an old mandelbox image, and needs to be cleaned and rebuilt.

## Styling

We use [Hadolint](https://github.com/hadolint/hadolint) to format the Dockerfiles in this project. Your first need to install Hadolint via your local package manager, i.e. `brew install hadolint`, and have the Docker daemon running before linting a specific file by running `hadolint <file-path>`.

We also have [pre-commit hooks](https://pre-commit.com/) with Hadolint support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` in the project folder, to instantiate the hooks for Hadolint. Dockerfile improvements will be printed to the terminal for all Dockerfiles specified under `args` in `.pre-commit-config.yaml`. If you need/want to add other Dockerfiles, you need to specify them there. If you have issues with Hadolint tracking non-Docker files, you can commit with `git commit -m [MESSAGE] --no-verify` to skip the pre-commit hook for files you don't want to track.

## FAQ

### I have the correct IP and ports for a mandelbox on my EC2 dev instance to connect to, but it looks like my protocol server and client aren't seeing each other!

Make sure your dev instance has a security group that has at least the ports [1025, 49151) open for incoming TCP and UDP connections.

### My Nvidia drivers stopped working!

To fix this, you can either (a) disable rolling kernel updates or (b) hardcode the kernel version in `/etc/default/grub` and refresh GRUB. We suggest you do B.
