# USAGE: docker build --file docker/Dockerfile ./
# - This assumes the build context is the repo root (see references to requirements.txt)

# Use official Python docker image to create the build artifacts.
# - The multi-stage pattern is being used since various python dependencies require compilation
#   to occur and we don't want to include the compiler toolchain in the final image.
# - Normally Alpine would be a good choice, but by using Debian (e.g. slim-buster)
#   it's easier to be compatible out of the box with various python dependencies
FROM python:3.8.3-slim-buster AS builder

# Create and change the repo directory
WORKDIR /repo

# Setup toolchain necessary for compilation activities in python dependencies
# hadolint ignore=DL3008,DL3015
RUN apt-get update \
 && apt-get install -y \
            build-essential \
            libssl-dev \
            libffi-dev \
            python-dev

# Install python requirements
COPY requirements.txt ./
RUN pip install -r requirements.txt

# Use official Python docker image as base.
# - Specify full Python version to avoid disparities in images (i.e. if someone
#   builds this package after someone else when a new python version was released in
#   between)
FROM python:3.8.3-slim-buster

# Copy fully compiled python dependencies from the builder image into this final
# image
COPY --from=builder /usr/local/lib/python3.8/site-packages /usr/local/lib/python3.8/site-packages
# Copy the bin tools created for the python dependencies from the builder image
# into this final image to ease developer usage
COPY --from=builder /usr/local/bin /usr/local/bin

# Create and change to the app directory.
WORKDIR /app

# Copy local code to the container image
# - This is split out in an attempt to improve Docker cache hits so that the full
#   dependency installation step doesn't need to be run again.
# - This expects .dockerignore to omit anything unnecessary for the image
COPY ./ ./

ENTRYPOINT ["./stem-cell.sh"]