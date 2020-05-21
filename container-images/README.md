# Fractal Container Images

This repository contains the base Docker images for the various single-application streaming that Fractal supports. Each subfolder contains its image for streaming the specific application.

## Existing Images

Here is a list of the existing Fractal single-application images, alongside their base container OS.

- Google Chrome | Ubuntu 18.04

## Development

If you are contributing to this repository, you can follow the structure of the `chrome` subfolder. Make sure to make the container image as lean as possible. This is not yet being done, but ideally we would soon start specifying maximum computing resources limits in the container image, based on a few tests, so we can ensure can condense the largest number of containers on the same hardware.
