# USAGE: docker build --file docker/Dockerfile ./
# - This assumes the build context is the webserver/ subrepo (see references to requirements.txt)

# Use official Python docker image to create the build artifacts.
# - The multi-stage pattern is being used since various Python dependencies require compilation
#   to occur and we don't want to include the compiler toolchain in the final image.
# - Normally Alpine would be a good choice, but by using Debian (e.g. slim-buster)
#   it's easier to be compatible out of the box with various Python dependencies
FROM python:3.9.5-slim-buster AS builder
# python:3.9.5 matches the python version running on Heroku as specified in runtime.txt.


# Create and change the repo directory
WORKDIR /repo

# Setup toolchain necessary for compilation activities in Python dependencies
# hadolint ignore=DL3008,DL3015
RUN apt-get update \
    && apt-get install -y \
    build-essential \
    libssl-dev \
    libffi-dev \
    python-dev \
    libpq-dev

# Install Python requirements
COPY requirements.txt ./
RUN pip install --no-cache-dir -r requirements.txt

# Use official Python docker image as base.
# - Specify full Python version to avoid disparities in images (i.e. if someone
#   builds this package after someone else when a new Python version was released in
#   between)
FROM python:3.9.5-slim-buster

# Copy fully compiled Python dependencies from the builder image into this final image
COPY --from=builder /usr/local/lib/python3.9/site-packages /usr/local/lib/python3.9/site-packages
# Copy the bin tools created for the Python dependencies from the builder image
# into this final image to ease developer usage
COPY --from=builder /usr/local/bin /usr/local/bin

# Change to the app directory, which is a mounted Docker volume.
WORKDIR /app

# Creates a non-root user and adds permission to access the /app folder.
# This provides privilage escalation mitigation benefits.
RUN useradd appuser && chown -R appuser /app
USER appuser

ENTRYPOINT ["./run-web.sh"]
