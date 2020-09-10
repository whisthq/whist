# Google Chrome Container Image

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
