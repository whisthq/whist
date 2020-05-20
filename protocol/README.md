# Fractal Protocol

This repository contains the source code for the Fractal Protocol, which streams audio and video at ultra-low latency from a virtual machine, OS container or regular computer, and streams back user actions and files.

### Style

For .c and .h files, we are formatting using the clang format `{BasedOnStyle: Google, IndentWidth: 4}`. If using VSCode or Visual Studio, please set this up in your editor to format on save if possible (in Visual Studio, this is through the C/C++ extension settings, as well as the general 'Format on Save' setting/shortcut). Otherwise, please make sure to run your code through `clang` before commits.
We also have a custom build tagret in the CMake 'clang-format' with will run with this style over all .c and .h files in server/ desktop/ and fractal/

------------

### Server

This folder contains the code to run Fractal servers that are streamed to any of the clients listed below. The currently-supported servers are:

-   Windows
-   Linux Ubuntu

### Desktop

This folder contains the code to run Fractal clients on desktop operating systems. The currently-supported servers are:

-   Windows
-   MacOS
-   Linux

### Android/Chromebook

TBD.

### iOS/iPadOS

TBD.

### Web

TBD.
 
 ------------
 ## GitHub Actions
 
 We have GitHub Actions enabled on this repository, in conjunction with CMake. Therefore, every time there is a push or a PR, GitHub will automatically compile both FractalClient and FractalServer on Windows, MacOS and Linux Ubuntu, and will outline any warnings and/or errors on the "Actions" tab above. Make sure to verify there after your push or PR.
 
  ------------

 ## Building
 If you are on ubuntu/linux run desktop/linux-client-setup.sh to install the dev versions of the dependencies. 
 ### Building in an IDE
 We use CMake to build. If you are using VS code, VS or Clion this is pretty easy to use, 
 you need to either open the root repo folder as a project, or open the root CMakeCache.txt
 as a project. On CLion and VS there is a menu to build at the top, on VS code you need the 
 CMake extension and the build command is at the bottom. The CMake currently has two types of builds
 Debug and Release. You probably want to be building debug builds while developing, since they log more
 aggressively (info and above). Currently, we use the same compiler flags for debug and release because we 
  distribute binaries with debug flags. The build target for desktop is "FractalClient" and the sever is "Fractal Server".
 
 
 #### Linux CLI
 Install cmake and ccmake, ccmake is a TUI for configuring the build. 
 From the root of the repo run ccmake ., you will initially see a blank screen because no cache has been built yet.
 Hit c to configure, this will populate the cache and show you a page with various settings. The three you care about 
 are BUILD_CLIENT, BUILD_SERVER which are both ON or OFF, and CMAKE_BUILD_TYPE which is one of Debug or Release.
 Next hit c again to reconfigure with your possibly new settings, then hit g to generate the makefile. This makefile has all of the build targets,
 including FractalClient, FractalServer and all of our libraries. It also includes CMake targets such as clean, edit_cache and rebuild cache.
 Running just make defaults to building FractalClient and FractalServer if you set both of these to ON in your configuration.
 GCC only supports one type of build at a time, so if you are currently building release, but want to build debug, you need to edit the cache and regenerate the makefile.  
 
 If you would like to keep your repo cleaner, make a build folder in protocol and run cmake from there. e.g cd "build && 
 cmake ../." this way you can just delete the contents of the build folder to start over. 
 
 #### Windows CLI
To build on windows use the command `cmake -G "NMake Makefiles"` at the root directory. This tells CMake to generate 
NMake style makefiles. Then, run `nmake` in either `server` or `desktop`, depending on which one you want to compile.

#### CMake installation 
##### Linux
If you are using a rolling release distro, e.g arch then you can likely install the newest version using pacman or your 
disto's package manager. 
If you are running 20.04 the version in the ubuntu package lists is fine. 
If you are running 18.04 the package lists only has 3.11.  You can install the newest version from the developer with 
```
sudo apt-get install apt-transport-https ca-certificates gnupg software-properties-common wget
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
sudo apt-get update
sudo apt-get install cmake
```
##### Windows
The latest binaries are available [here.](https://cmake.org/download/) 
#### Further documentation
More documentation is in our [gdrive](https://docs.google.com/document/d/1T9Lc3HVoqhqSjdUbiaFaQU71oV1VH25iFGDNvAYjtOs/edit?usp=sharing)
