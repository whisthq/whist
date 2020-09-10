# Fractal Base Container Image

| Ubuntu 18.04 | Ubuntu 20.04
|:--:|:--:|
|![Docker Image CI](https://github.com/fractalcomputers/container-images/workflows/Docker%20Image%20CI/badge.svg)|![Docker Image CI](https://github.com/fractalcomputers/container-images/workflows/Docker%20Image%20CI/badge.svg)|

This subfolder contains the base container images used to containerize specific applications and stream them via Fractal. These container images are responsible for containerizing the Fractal protocol and setting the core settings and structure needed to make Fractal run optimally on containers, before any application-specific configuration is required.

**Operating Systems Supported**

- Ubuntu 18.04
- Ubuntu 20.04

## Development




For reference on the bash functions called in the Docker image, refer to: https://github.com/fractalcomputers/setup-scripts

For reference on the Fractal protocol and streaming, refer to: https://github.com/fractalcomputers/protocol






You can run the base image via the `run.sh` script. It takes in a few parameters, `APP`, which determines the name of the folder/app to build, `VERSION==18|20` which specificies the Ubuntu version, anad `PROTOCOL` which specifies the local Fractal protocol directory.

```
# Usage
run.sh APP VERSION PROTOCOL

# Example
run.sh base 18 ../protocol
```

For more details on how to build and develop the base container image, see `/base/README.md`. 

We have basic continuous integration set for each container image through GitHub Actions. At every PR to `master` or `dev`, the Docker images will be built to ensure they work, and the status badges are listed in the respective subfolders' READMEs. You should make sure that your code passes all tests under the Actions tab, and that you add to the continuous integration if you add support for new applications and generate new Dockerfiles.






If you are contributing to this repository, you can follow the structure of the `chrome` subfolder. Make sure to make the container image as lean as possible, as we want only the strict minimum per container so that we can pack as many as possible on a single unit of hardware.

You should add new Bash functions for your container in the [setup-scripts repository](https://github.com/fractalcomputers/setup-scripts), and import them into your Dockerfile. The setup-scripts repository is linked here via a Git submodule in each container image repository, which you first need to update to the latest commit before using, by running in the appropriate subfolder:

```
git submodule update --init --recursive && cd setup-scripts && git pull origin master && cd ..
```

If you get access denied issues, you need to set-up your local SSH key with your GitHub account, which you can do by following this [tutorial](https://help.github.com/en/github/authenticating-to-github/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent). After running this command, you will have latest the setup-scripts code locally and can call the files as normal.

We have basic continuous integration set for each container image through GitHub Actions. At every push or PR to master, the Docker images will be built to ensure they work, and the status badges are listed in the respective subfolders' READMEs. You should make sure that your code passes all tests under the Actions tab.

