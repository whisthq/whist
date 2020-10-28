# Fractal Figma Linux Font Bindings

This repository contains the Linux bindings for the Figma fonts of the desktop version of Figma, enabling the MacOS version to be unpackaged and repackaged on Linux. These custom bindings are necessary to enable Figma to work as a desktop application on Linux to run it in one of our containers.

This project gets built by the Figma `Dockerfile.20` in the [`container-images`](https://github.com/fractalcomputers/container-images) repository, where it is linked as a submodule. As such, it does not need any deployment process itself.
