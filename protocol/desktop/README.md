## Fractal Desktop Client





This code launches a desktop (Windows, MacOS, Linux) into a client initiating a connection to their cloud virtual machine for interactive usage over the network.

### Development

You can run `desktop.bat` (Windows) or `desktop.sh` to restart your dev environment.

### Running

From the /desktop directory, you can simply run the following:

```desktop [IP ADDRESS] [[OPTIONAL] WIDTH] [[OPTIONAL] HEIGHT] [[OPTIONAL] MAX BITRATE]```

Alternatively, if you want to run the code directly, in the /desktop/build64 directory or /desktop/build32, call:

```FractalClient [IP ADDRESS] [[OPTIONAL] WIDTH] [[OPTIONAL] HEIGHT] [[OPTIONAL] MAX BITRATE]```


### Building (Depreciated)
It should be easily built via `make` (on MacOS with clang and Linux Ubuntu with gcc) or `nmake` (on Windows with MSVC - requires Visual Studio Desktop Development Extension). Tested with `Visual Studio Community Edition 2019`.
