# Fractal Protocol

|Master Status|Dev Status|
|:--:|:--:|
|![CMake Build Matrix](https://github.com/fractalcomputers/protocol/workflows/CMake%20Build%20Matrix/badge.svg?branch=master)|![CMake Build Matrix](https://github.com/fractalcomputers/protocol/workflows/CMake%20Build%20Matrix/badge.svg?branch=dev)|

This repository contains the source code for the Fractal Protocol, which is a low-latency interactive streaming technology that streams audio/video/actions between an OS-like device, whether it is a virtual machine, a container or a regular computer, and a client computer or mobile device.

## Supported Platforms

### Server

This folder builds a server to stream via Fractal. It supports Windows and Linux Ubuntu. See below for basic build instructions, and the folder's README for build and development details.

### Desktop

This folder builds a client to receive a server stream via Fractal. It supports Windows, MacOS and Linux Ubuntu. See below for basic build instructions, and the folder's README for build and development details.

### Android

This folder builds a client to receive a server stream via Fractal on Android-based devices, including Chromebooks. See the folder's README for build and development details.

### iOS

TBD.

### Web

TBD.

## Development

We use Cmake to compile, format and run tests. You first need to make sure Cmake 3.15 or higher is installed on your system. We also use cppcheck to run static tests on the code, which you should install as well.

### Installation

#### Linux

If you are using a rolling release distro, e.g. Arch then you can likely install the newest version using pacman or your 
disto's package manager. 
If you are running 20.04 the version in the Ubuntu package lists is fine. 
If you are running 18.04 the package lists only has 3.11.  You can install the newest version from the developer with 
```
sudo apt-get install apt-transport-https ca-certificates gnupg software-properties-common wget -y
wget -O - https://apt.kitware.com/keys/kitware-archive-latest.asc 2>/dev/null | sudo apt-key add -
sudo apt-add-repository 'deb https://apt.kitware.com/ubuntu/ bionic main'
sudo apt-get update
sudo apt-get install cmake -y
```

You can install cppcheck via your package manager, e.g. `apt-get install cppcheck`.

#### MacOS

You can install both Cmake and cppcheck via Homebrew.
```
brew install cmake

brew install cppcheck
```

##### Windows

On Windows, in addition to downloading Cmake, of which you can find the latest binaries [here.](https://cmake.org/download/), and cppcheck, which you can install via Chocolatey by running `choco install cppcheck --force`, you also need to install [Microsoft Visual Studio Community 2019](https://visualstudio.microsoft.com/downloads/) and select `Desktop Development with C++` add-on in the installer. This will install different Visual Studio Command Prompts for different architectures. In order to compile the protocol, you need to make sure to be using x86_x64 Visual Studio Developer Command Prompt.

### Building






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


#### Further documentation
More documentation is in our [gdrive](https://docs.google.com/document/d/1T9Lc3HVoqhqSjdUbiaFaQU71oV1VH25iFGDNvAYjtOs/edit?usp=sharing)













 
 ------------
 ## GitHub Actions
 
 We have GitHub Actions enabled on this repository, in conjunction with CMake. Therefore, every time there is a push or a PR, GitHub will automatically compile both FractalClient and FractalServer on Windows, MacOS and Linux Ubuntu, and will outline any warnings and/or errors on the "Actions" tab above. Make sure to verify there after your push or PR. The relevant workflow file for compiling is .github/workflow/build.yaml.
 This workflow also runs cppcheck on linux and Mac (windows is a possible todo), which is a static analysis tool which can catch errors which compilers cannot. e.g accessing uninitialized memory and other undefined behaviour. Not everything it catches is critical, but it does indicate the possibility of unexpected behaviour. 
 To see the warnings in context go to the actions tab, click on your PR/push that launched the action, select an OS it ran on and then select build. This expands the build log. 
 
 Dev status: 
 
 Master Status: 
 
  ------------










## Styling

For `.c` and `.h` files, we are formatting using the clang format `{BasedOnStyle: Google, IndentWidth: 4}`. You can download clang-format via the package manager for your respective OS:

```
MacOS:
brew install clang-format

Linux Ubuntu:
apt-get install clang-format

Windows:
choco install llvm --force
```

If using VSCode or Visual Studio, please set this up in your editor to format on save if possible (in Visual Studio, this is through the C/C++ extension settings, as well as the general 'Format on Save' setting/shortcut). Otherwise, please make sure to run your code through `clang` before commits. You can run it from the CLI by running `clang-format <file>`.

We also have a custom build tagret in the CMake 'clang-format' with will run with this style over all `.c` and `.h` files in `server/` `desktop/` and `fractal/`.
