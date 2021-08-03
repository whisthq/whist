## FUSE in Mandelboxes

This project contains the code to integrate a FUSE filesystem into a C program running inside a mandelbox. It uses `hello.c` from the examples folder in [the libfuse repo](https://github.com/libfuse/libfuse).

### Preliminaries

This project assumes running in an environment where an AppArmor config is available which allows the `mount` syscall, and the Seccomp filter enables `mount`, `umount`, `umount2`, and `unshare`. See https://github.com/fractal/fractal/pull/2930 for an example of setting up such an environment.

### Building

The docker container depends on the `fractal/base:current-build` image from https://github.com/fractal/fractal being built on the system. Then, simply run
```bash
docker build . --tag fractal/mandelbox-utils/fuse:current-build
```
to build the image.

### Running

The best way to run this image is from the `container-images` project in https://github.com/fractal/fractal. Simply run
```bash
./run_local_container_image mandelbox-utils/fuse
```
