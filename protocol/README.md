# Fractal Protocol

|Master Status|Dev Status|
|:--:|:--:|
|![CMake Build Matrix](https://github.com/fractalcomputers/protocol/workflows/CMake%20Build%20Matrix/badge.svg?branch=master)|![CMake Build Matrix](https://github.com/fractalcomputers/protocol/workflows/CMake%20Build%20Matrix/badge.svg?branch=dev)|

![Dev Nightly testing](https://github.com/fractalcomputers/protocol/workflows/Nightly%20testing/badge.svg)

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
brew install cmake cppcheck
```

##### Windows

First you will have to install [gitbash](https://git-scm.com/downloads). You can install cmake with the latest binaries [here](https://cmake.org/download/), and cppcheck, which you can install via Chocolatey by running `choco install cppcheck --force`. You also need to install [Microsoft Visual Studio Community 2019](https://visualstudio.microsoft.com/downloads/) and select `Desktop Development with C++` add-on in the installer. This will install different Visual Studio Command Prompts for different architectures. In order to compile the protocol, you need to make sure to be using x86_x64 Visual Studio Developer Command Prompt.

### Building

If you are on Linux Ubuntu, run `desktop/linux-client-setup.sh` to install the system dependencies.

#### IDE

We use CMake to build. If you are using VS code, VS or Clion, this is pretty easy to use. You need to either open the root repo folder as a project, or open the root `CMakelist.txt` as a project. On CLion and VS there is a menu to build at the top, on VS code you need the CMake extension and the build command is at the bottom. CMake currently has two types of builds, Debug and Release. You probably want to be building debug builds while developing, since they log more aggressively (Warning levels: Info and above). 

Currently, we use the same compiler flags for Debug and Release because we 
distribute binaries with debug flags, to better troubleshoot errors and bugs. 

The build target for desktop is "FractalClient" and the sever is "Fractal Server".

#### MacOS CLI

You can simply run `cmake .` from the root folder, `/protocol/`, which will generate the makefiles. You can then run `make FractalClient` from the root folder, or cd into `/desktop` and run `make` to compile the MacOS client.

#### Linux CLI

Install cmake and ccmake; ccmake is a TUI for configuring the build. From the root of the repo run `ccmake .`. You will initially see a blank screen because no cache has been built yet. Hit `c` to configure. This will populate the cache and show you a page with various settings. The three you care about are `BUILD_CLIENT` and `BUILD_SERVER` which are both `ON` or `OFF`, and `CMAKE_BUILD_TYPE` which is one of `Debug` or `Release`.

Next hit `c` again to reconfigure with your possibly new settings, then hit `g` to generate the makefile. This makefile has all of the build targets, including FractalClient, FractalServer and all of our libraries. It also includes CMake targets such as clean, edit_cache and rebuild cache.

Only running `make` defaults to building FractalClient and FractalServer if you set both of these to ON in your configuration.
GCC only supports one type of build at a time, so if you are currently building Release, but want to build Debug, you need to edit the cache and regenerate the makefile.  

If you would like to keep your repo cleaner, make a build folder in `/protocol` and run `cmake` from there, e.g. `cd "build & cmake ../."`. This way, you can just delete the contents of the build folder to start over. 

#### Windows CLI

To build on Windows, run the command `cmake -G "NMake Makefiles"` at the root directory from an x86_64 Visual Studio Developper Command Prompt, which is obtained through downloading Visual Studio. This tells CMake to generate 
NMake-style makefiles. Then, run `nmake` in either `/server` or `/desktop`, depending on which one you want to compile.

#### Further documentation

More documentation is in our [Google Drive](https://docs.google.com/document/d/1T9Lc3HVoqhqSjdUbiaFaQU71oV1VH25iFGDNvAYjtOs/edit?usp=sharing), if needed. We also use Doxygen in this repository. The Doxy file is `docs/Doxyfile`. To generate it, you should first install doxygen via your local package manager, and run `doxygen Doxyfile`. This will generate the docs and put them in `docs/html` and `docs/latex`. You can view the html docs by opening the index.html page with a web browser. We keep the docs gitignored to avoid clutter on the repository, since we don't publish them anywhere.

## CI & CD

### Continuous Deployment

Every commit to `dev`, `staging` and `master`, as well as each new PR opened or updated, results in a new build being created an uploaded to GitHub Releases. These builds are versioned using the following scheme `GITREF-YYYYMMDD.#`, where
- `GITREF` is either the branch name (such as `dev`) or the PR info (such as `pr255merge` for the merge commit on PR #255)
- `YYYYMMDD` is the current UTC date
- `#` is an incrementing integer to disambiguate multiple releases on the same date

A build of the `client-applications` will also be triggered for each new `protocol` build. See that repository's README for more information.

#### Checks & Tests

These builds will also have `cppcheck` run against them which is a static analysis tool which can catch errors which compilers cannot, e.g. accessing uninitialized memory and other undefined behavior. Not everything it catches is critical, but it does indicate the possibility of unexpected behavior.

These builds will also (TODO) be tested against a live server VM. This workflow will spin up an Azure VM, upload the server build to it, and then use GitHub Actions VMs on Windows, MacOS and Linux Ubuntu as clients to connect and stream via the protocol for one minute. This will also occur nightly against the `dev` branch, but these builds will not be released (this can be removed once testing is stable and re-enabled on all commits).

To see the warnings in context go to the Actions tab, click on your PR/push that launched the action, select an OS it ran on and then select build. This expands the build log, where you can clearly see the warnings/errors generated. 

### Special Cases

You can add `WIP` anywhere in the title of a PR to keep the CD auto-build workflow from running

You can include `skip-ci` anywhere in a commit message to `dev`, `staging`, or `master` to keep the CD auto-build workflow from running.

### Continuous Integration

For every push or PR, the code will be automatically linted via clang-format according to the [styling](#styling) guidelines.

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

If using VSCode or Visual Studio, please set this up in your editor to format on save if possible (in Visual Studio, this is through the C/C++ extension settings, as well as the general 'Format on Save' setting/shortcut). Otherwise, please make sure to run your code through `clang` before commits. You can run it from the CLI by running `cmake . && make clang-format`, on MacOS/Linux, or `cmake -G "NMake Makefiles" && make clang-format`, on Windows.

We also have a custom build tagret in the CMake 'clang-format' with will run with this style over all `.c` and `.h` files in `server/` `desktop/` and `fractal/`.
