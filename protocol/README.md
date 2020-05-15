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
 ## Building
 ### Building in an IDE
 We use CMake to build. If you are using VS code, VS or Clion this is pretty easy to use, 
 you need to either open the root repo folder as a project, or open the root CMakeCache.txt
 as a project. On CLion and VS there is a menu to build at the top, on VS code you need the 
 CMake extension and the build command is at the bottom. The CMake currently has two types of builds
 Debug and Release. You probably want to be building debug builds while developing, since they log more
 aggressively (info and above) and add debug compile flags such as -g. The build target for desktop is "FractalClient"
 
 
 #### Linux CLI
 Install cmake and ccmake, ccmake is a TUI for configuring the build. 
 From the root of the repo run ccmake ., you will initially see a blank screen because no cache has been built yet.
 Hit c to configure, this will populate the cache and show you a page with various settings. The three you care about 
 are BUILD_CLIENT, BUILD_SERVER which are both ON or OFF, and CMAKE_BUILD_TYPE which is one of Debug or Release.
 Next hit c again to reconfigure with your possibly new settings, then hit g to generate the makefile. This makefile has all of the build targets,
 including FractalClient, FractalServer and all of our libraries. It also includes CMake targets such as clean, edit_cache and rebuild cache.
 Running just make defaults to building FractalClient and FractalServer if you set both of these to ON in your configuration.
 GCC only supports one type of build at a time, so if you are currently building release, but want to build debug, you need to edit the cache and regenerate the makefile.  
 
 #### Windows CLI
 I have not yet built from the commandline on windows, but there is a gui utility which is basically the same as ccmake. 
 I imagine it is very similar to the Linux process once you have this gui open, but it will create nmake style make files.  
