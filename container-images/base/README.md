# Fractal Base Container Image

| Ubuntu 18.04 | Ubuntu 20.04
|:--:|:--:|
|![Docker Image CI](https://github.com/fractalcomputers/container-images/workflows/Docker%20Image%20CI/badge.svg)|![Docker Image CI](https://github.com/fractalcomputers/container-images/workflows/Docker%20Image%20CI/badge.svg)|

This subfolder contains the base container images used to containerize specific applications and stream them via Fractal, alongside the Linux services they need to run properly. These container images are responsible for containerizing the Fractal protocol and setting the core settings and structure needed to make Fractal run optimally on containers, before any application-specific configuration is required.

**Operating Systems Supported**

- Ubuntu 18.04
- Ubuntu 20.04

## Development

This repository calls the Fractal [setup-scripts](https://github.com/fractalcomputers/setup-scripts) repository as a Git submodule for most of the Bash functions needed to set up Fractal. New functions you make should be added to that repository so they can be easily reused across the Fractal organization. Before using these functions in this project, you need to update the Git submodule linked by running:

```
git submodule update --init --recursive && cd setup-scripts && git pull origin master && cd ..
```

If you get access denied issues, you need to set-up your local SSH key with your GitHub account, which you can do by following this [tutorial](https://help.github.com/en/github/authenticating-to-github/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent). After running this command, you will have latest the setup-scripts code locally and can call the files as normal.

Any services needed for the containers to function properly should be developed and hosted as part of this project. For any details on continuous integration and styling, refer to the outer README of this project.

## Running

You can run the base image via the `run.sh` script. It takes in a few parameters, `APP`, which determines the name of the folder/app to build, `VERSION==18|20` which specificies the Ubuntu version, anad `PROTOCOL` which specifies the local Fractal protocol directory.

```
# Usage
../run.sh APP VERSION PROTOCOL

# Example
../run.sh base 18 ../protocol
```
