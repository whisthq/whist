FROM fractal/base:current-build

# Force failure on non-zero exit code for commands with a pipe in them
SHELL ["/bin/bash", "-Eeuo", "pipefail", "-c"]

# Install FUSE build dependencies
RUN apt-get update && apt-get install --allow-downgrades --no-install-recommends -y \
    gcc \
    vim \
    fuse3 \
    libfuse3-dev \
    pkg-config \
    build-essential \
    strace \
    gdb \
    && apt-get autoremove -y \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN mkdir -p /usr/share/fractal-fuse

COPY hello.c /usr/share/fractal-fuse

RUN cd /usr/share/fractal-fuse \
    && gcc -Wall -D _GNU_SOURCE hello.c `pkg-config fuse3 --cflags --libs` -o hello \
    && cp hello /usr/bin/fractal-fuse

COPY unshare.c /usr/share/fractal-fuse

RUN cd /usr/share/fractal-fuse \
    && gcc -Wall -D _GNU_SOURCE unshare.c -o unshare \
    && cp unshare /usr/bin/fractal-unshare
