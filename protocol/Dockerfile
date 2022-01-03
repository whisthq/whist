FROM ubuntu:20.04
ARG uid

# This Dockerfile is used to build the Whist protocol server for Linux Ubuntu 20.04.

# Force failure on non-zero exit code for commands with a pipe in them
SHELL ["/bin/bash", "-Eeuo", "pipefail", "-c"]

# Set protocol builder environment variables
ENV container docker
ENV LC_ALL C
ENV DEBIAN_FRONTEND noninteractive
RUN sed -i 's/# deb/deb/g' /etc/apt/sources.list

# Turn off interactive mode
RUN echo 'debconf debconf/frontend select Noninteractive' | debconf-set-selections

# Install and configure Ubuntu utils
RUN apt-get update && apt-get install --allow-downgrades --no-install-recommends -y \
    apt-utils \
    wget \
    curl \
    git \
    xz-utils \
    apt-transport-https \
    ca-certificates \
    gnupg \
    software-properties-common \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN APT_KEY_DONT_WARN_ON_DANGEROUS_USAGE=1 \ 
    wget -qO - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | apt-key add -
    
RUN apt-add-repository 'deb https://apt.kitware.com/ubuntu/ focal main'

RUN apt-get update \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN dpkg --configure -a && apt-get install --allow-downgrades --no-install-recommends -y -f

# Install build tools & `docs` target dependencies
RUN apt-get update && apt-get install --allow-downgrades --no-install-recommends -y \
    # build tools
    make \
    gcc \
    g++ \
    cppcheck \
    cmake=3.18.4-0kitware1ubuntu20.04.1 \
    cmake-data=3.18.4-0kitware1ubuntu20.04.1 \
    sudo \
    clang-format \
    awscli \
    lcov \
    jq \
    build-essential \
    # `docs` target dependencies
    doxygen \
    coreutils \
    perl \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN echo "Set disable_coredump false" >> /etc/sudo.conf

# Install Whist protocol Linux dependencies
COPY setup-linux-build-environment.sh /
RUN apt-get update \
    && ./setup-linux-build-environment.sh \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install dbus utilities
RUN apt-get update && apt-get install -y \
    dbus-x11 \
    libdbus-1-dev \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install clang-tidy 6. We must hardcode this version, else we will install the
# overaggressive recent version, and we need to symlink it to be able to call
# it as "clang-tidy".
RUN mkdir /clang-tidy \
    && wget -q -O - https://releases.llvm.org/6.0.1/clang+llvm-6.0.1-x86_64-linux-gnu-ubuntu-16.04.tar.xz | tar -xJ -C /clang-tidy \
    && ln -sf "/clang-tidy/clang+llvm-6.0.1-x86_64-linux-gnu-ubuntu-16.04/bin/clang-tidy" "/usr/bin/clang-tidy"

# Create whist Linux user for building the protocol
RUN groupadd whist \
    && useradd -lou ${uid} -g whist whist-builder \
    && usermod -aG sudo whist-builder \
    && echo "whist-builder:password" \
    | chpasswd

# Specify workdir for building the protocol
WORKDIR /workdir
