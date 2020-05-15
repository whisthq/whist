## Fractal Windows/Linux Ubuntu Server

This code turns a Windows or Linux Ubuntu computer into a server listening to receive a client connection to stream its desktop to at ultra-low latency over the Internet.
### Building in an IDE
We use CMake to build. If you are using VS code, VS or Clion this is pretty easy to use, 
you need to either open the root repo folder as a project, or open the root CMakeCache.txt
as a project. On CLion and VS there is a menu to build at the top, on VS code you need the 
CMake extension and the build command is at the bottom. The CMake currently has two types of builds
Debug and Release. You probably want to be building debug builds while developing, since they log more
aggressively (info and above) and add debug compile flags such as -g. The build target for desktop is "FractalClient"

### Building from the command line


### Development

You can run `server.bat` (Windows) or `server.sh` to restart your dev environment.

### Running

```FractalServer```

### Updating

You can update all Windows virtual machines by running `update.bat`, and all Linux Ubuntu virtual machines by running `update.sh`. This action is only allowed from the stable production branch and needs to be coordinated with the client updating.

### Building (depreciated)
It should be easily built via `make` (on Linux Ubuntu with gcc) or `nmake` (on Windows with MSVC - requires Visual Studio Desktop Development Extension). Tested with `Visual Studio Community Edition 2019`.

