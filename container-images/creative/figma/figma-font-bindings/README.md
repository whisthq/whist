# Fractal Figma Linux Font Bindings

Originally forked from [https://github.com/Figma-Linux/figma-linux-rust-binding](https://github.com/Figma-Linux/figma-linux-rust-binding).

This subfolder contains the Linux Ubuntu bindings for the Figma fonts of the desktop version of Figma. With these bindings, it is possible to unpackage the macOS Figma desktop application and run it on Linux Ubuntu, which enables us to stream the desktop version of Figma running in a container.

## Development

There is no active development on this project; we cloned it over and made a few small modifications to make it work on our version of Linux. If you need to modify it, just use common sense and keep the code clean. To see how to load it in the unpackaged macOS Figma application, see `creative/figma/Dockerfile.20`.

## Publishing

This project gets built into the Figma container image, via `container-images/creative/figma/Dockerfile.20`. Therefore, it gets automatically deployed through the `fractal-publish-build.yml` GitHub Actions workflow, under the `containers-and-ami-publish-ghcr-and-aws` job.
