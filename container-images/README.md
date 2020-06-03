# Fractal Container Images

This repository contains the base Docker images for the various single-application streaming that Fractal supports. Each subfolder contains its image for streaming the specific application using Fractal, with a fully self-contained container.

## Existing Images

Here is a list of the existing Fractal single-application images, alongside their base container OS:

TBD

## Development

If you are contributing to this repository, you can follow the structure of the `chrome` subfolder. Make sure to make the container image as lean as possible, as we want only the strict minimum per container so that we can pack as many as possible on a single unit of hardware.
