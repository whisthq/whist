# Fractal Figma Linux Font Bindings

(Originally forked from [https://github.com/Figma-Linux/figma-linux-rust-binding](https://github.com/Figma-Linux/figma-linux-rust-binding))

This subfolder contains the Linux bindings for the Figma fonts of the desktop version of Figma, enabling the macOS version to be unpackaged and run on Linux Ubuntu. These custom bindings are necessary to enable Figma to work as a desktop application on Linux to run it in one of our containers.

## Development



## Publishing

This project gets built by the Figma `Dockerfile.20`, where it is stored as a subfolder. As such, it does not need any deployment process itself, and gets automatically packaged in the Figma Dockerfile when it gets deployed to production via the 
