FROM ubuntu:21.04
ARG uid

# This Dockerfile is used to build the Fractal protocol server for Linux Ubuntu 21.04.

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
    xz-utils \
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

# Install build tools
RUN apt-get update && apt-get install --allow-downgrades --no-install-recommends -y \
    make \
    gcc \
    g++ \
    cppcheck \
    cmake \
    cmake-data \
    sudo \
    clang-format \
    awscli \
    jq \
    build-essential \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install `docs` target dependencies
RUN apt-get update && apt-get install --allow-downgrades --no-install-recommends -y \
    doxygen \
    coreutils \
    perl \
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

# Install clang-tidy 6. We must hardcode this version, else we will install the
# overaggressive recent version, and we need to symlink it to be able to call
# it as "clang-tidy".
RUN mkdir /clang-tidy \
    && wget -q -O - https://releases.llvm.org/6.0.1/clang+llvm-6.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz | tar -xJ -C /clang-tidy \
    && ln -sf "/clang-tidy/clang+llvm-6.0.1-x86_64-linux-gnu-ubuntu-16.04/bin/clang-tidy" "/usr/bin/clang-tidy"

# Create fractal Linux user for building the protocol
RUN groupadd fractal
RUN useradd -lou ${uid} -g fractal fractal-builder
RUN usermod -aG sudo fractal-builder
RUN echo "fractal-builder:password" | chpasswd

# Specify workdir for building the protocol
WORKDIR /workdir
