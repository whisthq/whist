# Whist Uinput Go

**This directory was originally forked from [bendahl/uinput](https://github.com/bendahl/uinput), and that repository was then copied into this directory of the host-service.**

We forked the original `uinput` go package because we needed to expose some more `ioctl` functionality that relies on implementation details of the package. In particular, we wish to [determine the files corresponding to a uinput device](https://stackoverflow.com/questions/15623442/how-do-i-determine-the-files-corresponding-to-a-uinput-device), which requires access to the `*os.File` objects that are created and hidden behind the input device type-specific interfaces in the package. Please keep modifications to this repository as minimal as possible.
