## FUSE in Mandelboxes

This project contains the code to integrate a FUSE filesystem into a C program running inside a mandelbox. It uses `hello.c` from the examples folder in [the libfuse repo](https://github.com/libfuse/libfuse).

### Preliminaries

This project assumes running in an environment where an AppArmor config is available which allows the `mount` syscall, and the Seccomp filter enables `mount`, `umount`, `umount2`, and `unshare`. See https://github.com/whisthq/whist/pull/2930 for an example of setting up such an environment.

### Building

The docker container depends on the `whist/base:current-build` image from https://github.com/whisthq/whist being built on the system. Then, simply run

```bash
./build.sh
```

to build the image (in this folder; not using the `mandelboxes/build.sh`).

### Running

The best way to run this image is from the `mandelboxes` project in https://github.com/whisthq/whist. Simply run:

```bash
./run.sh mandelboxes/development/utils/fuse
```
