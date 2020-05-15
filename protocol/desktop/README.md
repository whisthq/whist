## Fractal Desktop Client

This code launches a desktop (Windows, MacOS, Linux) into a client initiating a connection to their cloud virtual machine for interactive usage over the network.

### Building in an IDE
We use CMake to build. If you are using VS code, VS or Clion this is pretty easy to use, 
you need to either open the root repo folder as a project, or open the root CMakeCache.txt
as a project. On CLion and VS there is a menu to build at the top, on VS code you need the 
CMake extension and the build command is at the bottom. The CMake currently has two types of builds
Debug and Release. You probably want to be building debug builds while developing, since they log more
aggressively (info and above) and add debug compile flags such as -g. 

### Building from the command line

### Development

You can run `desktop.bat` (Windows) or `desktop.sh` to restart your dev environment.

### Running

From the /desktop directory, you can simply run the following:

```desktop [IP ADDRESS] [[OPTIONAL] WIDTH] [[OPTIONAL] HEIGHT] [[OPTIONAL] MAX BITRATE]```

Alternatively, if you want to run the code directly, in the /desktop/build64 directory or /desktop/build32, call:

```FractalClient [IP ADDRESS] [[OPTIONAL] WIDTH] [[OPTIONAL] HEIGHT] [[OPTIONAL] MAX BITRATE]```


### Building (Depreciated)
It should be easily built via `make` (on MacOS with clang and Linux Ubuntu with gcc) or `nmake` (on Windows with MSVC - requires Visual Studio Desktop Development Extension). Tested with `Visual Studio Community Edition 2019`.
