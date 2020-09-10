# Fractal Base Container Image

![Docker Image CI](https://github.com/fractalcomputers/container-images/workflows/Docker%20Image%20CI/badge.svg)

This folder contains the container image for the streaming Google Chrome via Fractal. The Dockerfile is based on an Ubuntu 18.04 image and installs all required 

For reference on the bash functions called in the Docker image, refer to: https://github.com/fractalcomputers/setup-scripts

For reference on the Fractal protocol and streaming, refer to: https://github.com/fractalcomputers/protocol

## Running

Building the container
`docker build -t chrome . `

To build and run, run the following in the root of the project
`./runchrome.sh`

The runchrome script contains fixes for networking issues when setting up the firewall.

Then open up vnc and connect it to 5900 with the VNC_SERVER_PASSWORD




If you are contributing to this repository, you can follow the structure of the `chrome` subfolder. Make sure to make the container image as lean as possible, as we want only the strict minimum per container so that we can pack as many as possible on a single unit of hardware.

You should add new Bash functions for your container in the [setup-scripts repository](https://github.com/fractalcomputers/setup-scripts), and import them into your Dockerfile. The setup-scripts repository is linked here via a Git submodule in each container image repository, which you first need to update to the latest commit before using, by running in the appropriate subfolder:

```
git submodule update --init --recursive && cd setup-scripts && git pull origin master && cd ..
```

If you get access denied issues, you need to set-up your local SSH key with your GitHub account, which you can do by following this [tutorial](https://help.github.com/en/github/authenticating-to-github/generating-a-new-ssh-key-and-adding-it-to-the-ssh-agent). After running this command, you will have latest the setup-scripts code locally and can call the files as normal.

We have basic continuous integration set for each container image through GitHub Actions. At every push or PR to master, the Docker images will be built to ensure they work, and the status badges are listed in the respective subfolders' READMEs. You should make sure that your code passes all tests under the Actions tab.

