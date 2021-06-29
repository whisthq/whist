FROM ubuntu:20.04
ARG uid

# This Dockerfile is used to build the Fractal protocol server for Linux Ubuntu 20.04.

# Force failure on non-zero exit code for commands with a pipe in them
SHELL ["/bin/bash", "-Eeuo", "pipefail", "-c"]

# Set protocol builder environment variables
ENV container docker
ENV LC_ALL C
ENV DEBIAN_FRONTEND noninteractive
RUN sed -i 's/# deb/deb/g' /etc/apt/sources.list

# Install and configure Ubuntu utils
RUN apt-get update && apt-get install --allow-downgrades --no-install-recommends -y \
    wget \
    apt-utils \
    curl \
    git \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install --allow-downgrades --no-install-recommends -y \
    apt-transport-https \
    ca-certificates \
    gnupg \
    software-properties-common \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | apt-key add -
RUN apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'

RUN apt-get update \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN dpkg --configure -a && apt-get install --allow-downgrades --no-install-recommends -y -f

# Install build tools
RUN apt-get update && apt-get install --allow-downgrades --no-install-recommends -y \
    make \
    gcc \
    g++ \
    cppcheck \
    cmake=3.18.4-0kitware1ubuntu20.04.1 \
    cmake-data=3.18.4-0kitware1ubuntu20.04.1 \
    sudo \
    clang-format \
    awscli \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN echo "Set disable_coredump false" >> /etc/sudo.conf

# Install Fractal protocol Linux dependencies
COPY setup-linux-build-environment.sh /
RUN apt-get update \
    && ./setup-linux-build-environment.sh \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Create fractal Linux user for building the protocol
RUN groupadd fractal
RUN useradd -lou ${uid} -g fractal fractal-builder
RUN usermod -aG sudo fractal-builder
RUN echo "fractal-builder:password" | chpasswd

# Specify workdir for building the protocol
WORKDIR /workdir
