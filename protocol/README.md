# Whist Protocol

This repository contains the source code for the Whist Protocol, which is a low-latency interactive streaming technology that streams audio/video/inputs between an OS-like device, whether it is a virtual machine, a mandelbox, or a regular computer, and a client computer or mobile device.

## Supported Platforms

### Server

This folder builds a server to stream via Whist. It supports Windows and Linux Ubuntu. See below for basic build instructions, and the folder's README for server-specific development details.

### Client

This folder builds a client to receive a server stream via Whist. It supports Windows, MacOS and Linux Ubuntu. See below for basic build instructions, and the folder's README for build and development details.

## High-Level Overview

In order to give the user their low-latency high-FPS experience, the protocol executes the following processes in order from top-to-bottom:

- `create_capture_device` from either `./whist/video/dxgicapture.c` or `./whist/video/x11capture.c` is called, the former is for Windows and the latter is for Linux. Then, `capture_screen` is called in `./server/main.c` to capture the screen.
- `encoder_t` from `videoencode.c` will be used to encode the screenshot using h264, if needed. If Nvidia Capture SDK is being used (`USING_GPU_CAPTURE == true` and on linux), then the image will already be encoded upon capture, and this is not necessary.
- A `VideoFrame*` is created and the members of the `VideoFrame` struct is filled in, see `./server/main.c` for the code and `./whist/core/whist_frame.h` for the `VideoFrame` struct.
- `broadcast_udp_packet_from_payload` from `./server/network.c` is called with the `VideoFrame*` passed in. This will break-up the `Frame*` into hundreds of individual network packets.
- On the client, these packets are received in `./client/main.c` and passed into `receive_video` in `./client/video.c`.
- `receive_video` will receive video packets and will save them in a buffer (packet 17 will be stored at `buffer + PACKET_SIZE*(17-1)`, so that the packets go into their correct slot until the entire `VideoFrame*` is recreated). `video.c` keeps track of the ID of the most recently rendered frame, ie ID 247. Once all of the packets of ID 248 are received, it will take the pointer to the beginning of the buffer and then render it. Each packet will contain the number of packets for the `VideoFrame*`, so once one is received, the client will know when all of them have been received.
- Once a `VideoFrame*` has been accumulated, `render_screen` in `./server/video.c` will trigger, and `video_decoder_decode` from `./whist/video/videodecode.c` will be called. Any cursor image will be rendered on-top, along with `sws_scale`'ing if the frame is not the same resolution as the client. Peruse `render_screen` for further details.
- Finally, `SDL_RenderPresent` will be called, rendering the Frame.
- If no packet from the expected ID is received within a couple hundred milliseconds, or if a subset of packets have been received and it's been a couple hundred milliseconds since the last seen packet, then the `./client/video.c`protocol will send a `nack()` to the server in order to get a copy of the presumably dropped or missed packet. The server keeps the last couple `Frame*`'s in a buffer in order to respond to `nack()`'s.

The same process is used for audio capture, encoding, decoding, and playing. See `send_audio` of `./server/main.c`, `./whist/audio/audio{capture,encode,decode}.c`, and `receive_audio` of `./client/audio.c`

Throughout the life of the protocol, various messages will be send to and from the client and server. For example, if the client and server are of different resolution, the image will appear stretched, so to fix this the client will send a message to server to ask the server to change resolutions to match the client.

- To send a message from client to server, call `send_fcmsg`. See `./client/main.c` for usage.
- To send a message from server to client, create a `WhistServerMessage` and call `broadcast_tcp_packet` or `broadcast_udp_packet`. See `./server/main.c` for usage.
- To handle a server message on the client, see `./client/handle_server_message.c`
- To handle a client message on the server, see `./server/handle_client_message.c`
- See `./whist/core/whist.h` for struct definitions.

Of course, input must also be sent from client to server. This is handled in the form of SDL Events, which are retrieved in `./client/main.c` and handled in `sdl_event_handler.c`. These generally take the form of `fcmsg`'s sent from client to server, over `UDP` for speed. We don't handle packet dropping, however, so sometimes the capslock and numlock will go out-of-sync. We use `sync_keyboard_state` to fix this, resyncing stateful keys every 50ms with an `fcmsg`. This additionally handles the initial sync by-default.

### Current Encoding Status

Our current status of using encoders is a bit tricky.

In an ideal world, we'd use the NVIDIA Capture SDK with the NVIDIA encoder. However, there's some nasty bugs in the currently available versions our our interfaces with them (namely device creation fails sometimes and hangs the protocol for 10 seconds). At the moment, we're using X11 capture with ffmpeg, which has been our fallback capturing mechanism. We are currently in the process of switching to X11 capture with direct NVIDIA encoding, since it's slightly faster and reconfigurable on the fly. If we can figure out how to safely switch back to NVIDIA capture as well, then we will do that for simplicity and performance.

## File Structure

`tree -I "share|sentry-native|lib|include|build*|CMakeFiles|docs|loading|cmake" -P "*.c" .`

```
./protocol
├── client
│   ├── audio.c <- Handle and play audio packets
│   ├── bitrate.c <- Protocols for calculating adaptive bitrate
│   ├── client_statistic.c <- Client-side metrics
│   ├── client_utils.c <- cmdline options, among others
│   ├── handle_server_message.c <- Handle server fcmsg's
│   ├── main.c <- SDL Event loop, receive and categorize packets as fcmsg/audio/video
│   ├── native_window_utils_windows.c <- Functions to modify SDL window on Windows
│   ├── native_window_utils_x11.c <- Functions to modify SDL window for X11
│   ├── network.c <- Functions to connect to server, and send_fcmsg
│   ├── sdl_event_handler.c <- Handle SDL Events
│   ├── sdlscreeninfo.c <- Get monitor/window width/height functions
│   ├── sdl_utils.c <- Set window icon, resize handler
│   ├── sync_packets.c <- Syncs info betwen client/server including clipboard data, files, and bitrate
│   └── video.c <- Handle and render video packets
├── whist
│   ├── audio
│   │   ├── alsacapture.c <- Capture Linux audio
│   │   ├── audiodecode.c <- AAC Decode Audio
│   │   ├── audioencode.c <- AAC Encode Audio
│   │   └── wasapicapture.c <- Capture Windows audio
│   ├── clipboard
│   │   ├── clipboard.c <- Wrappers for get_clipboard/set_clipboard
│   │   ├── clipboard_synchronizer.c <- Multithreaded wrapper for get/set clipboard,
│   │   │                                 this is what actually gets used
│   │   ├── mac_clipboard.c <- Mac implementation of {get,set}_clipboard
│   │   ├── win_clipboard.c <- Windows implementation of {get,set}_clipboard
│   │   └── x11_clipboard.c <- Linux implementation of {get,set}_clipboard
│   ├── core
│   │   ├── whist.c <- Various helpers
│   │   └── whist_frame.c <- VideoFrame and AudioFrame
│   │   ├── whistgetopt.c <- Cross-platform getopt
│   │   └── whist_memory.c <- Memory handling
│   ├── cursor
│   │   ├── linuxcursor.c <- whist_cursor_capture for Linux
│   │   └── windowscursor.c <- whist_cursor_capture for Windows
│   ├── file
│   │   ├── file_drop.c <- Handles dropping a file into window
│   │   └── file_synchronizer.c.c <- Sync file transfer between client and server
│   ├── input
│   │   ├── input.c <- Trigger keyboard/mouse press and sync, wraps raw input_driver.h calls
│   │   ├── input_driver.h -> The following three c files share this h file
│   │   ├── winapi_input_driver.c <- Windows keyboard/mouse-press code
│   │   ├── uinput_input_driver.c <- Linux uinput keyboard/mouse-press/touchpad code
│   │   └── xtest_input_driver.c <- Linux X11 keyboard/mouse-press code
│   ├── logging
│   │   ├── error_monitor.c <- Error reporting
│   │   └── logging.c.c <- Logging tools
│   │   └── log_statistic.c.c <- Logging tools for repeated stats
│   ├── network
│   │   └── network.c <- send udp/tcp/http packets.
│   │   └── ringbuffer.c <- Ring buffer for receiving audio/video
│   │   └── tcp.c <- Sending tcp packets
│   │   └── throttle.c <- Network throttler
│   │   └── udp.c <- Sending udp packets
│   ├── utils
│   │   ├── aes.c <- Generic encrypt/decrypt of network packets
│   │   ├── clock.c <- Clock
│   │   ├── color.c <- RGB/YUV conversation, and other color helpers
│   │   ├── error_monitor.c <- our interface submitting breadcrumbs and events to Sentry
│   │   ├── lodepng.c <- LodePNG external dependency
│   │   ├── logging.c <- LOG_INFO/LOG_ERROR/etc.
│   │   ├── mac_utils.c <- Mac wrappers
│   │   ├── png.c <- Functions to encode/decode bitmap's as png's
│   │   ├── rwlock.c <- Implementation of a lock with one writer and many readers
│   │   ├── sysinfo.c <- Print RAM/OS/Memory/HDD etc for logging purposes
│   │   ├── window_name.c <- Linux Getter for the window name of the application
│   │   └── windows_utils.c <- Log-in past windows start screen
│   └── video
│       ├── capture
│       │   └── linuxcapture.c <- Capture screen on linux
│       │   └── nvidiacaapture.c <- Screen capture via Nvidia FBC SDK
│       │   └── windowscapture.c <- Capture screen on Windows
│       │   └── x11capture.c <- Capture screen using X11
│       ├── codec
│       │   └── decode.c  <- Decode image
│       │   └── encode.c <- Compress image using h264/h265
│       │   └── ffmpeg_encode.c  <- FFmpeg encoding
│       │   └── nvidia_encode.c <- NVIDIA encoding
│       ├── cudacontext.c <- Set up CUDA context for Nvidia encoder
│       ├── dxgicapture.c <- Capture screen using DXGI
│       ├── nvidia-linux
│       │   └── NvFBCUtils.c (NVDA Header)
│       ├── transfercapture.c <- Implements CPU or CUDA transfer of frame from device to encoder
│       └── x11nvidiacapture.c <- Capture h264/h265 compressed screen with NvidiaCaptureSDK
├── server
│   ├── audio.c <- Send audio
│   ├── client.c <- Handle multiclient messages
│   ├── handle_client_message.c <- Handle client fmsg's
│   ├── main.c <- Initialize server, receive packets and pass to where it needs to go
│   ├── network.c <- Networking code for multiclient
│   ├── parse_args.c <- Arg parsing for server
│   ├── server_statistic.c <- Server-side metrics
│   └── video.c <- Send video
└── test
    ├── images <- images for unit testing
    └── protocol_test.cpp <- tests code in whist/ module
```

The above files are fairly static. If you add or remove a file, or change what a file does, please update this directory so we can keep track of it all!

## Development

We use Cmake >= 3.15 to compile, format and run tests on all systems, and also use Ninja on Windows for faster compilation. Make sure it is installed in your system (platform-specific instructions follow below).

### Installation of Dependencies

On all three platforms, you will need to install and log into the [AWS CLI](https://docs.aws.amazon.com/cli/latest/userguide/install-cliv2.html) before you can build the protocol. On command line, you should run `aws configure`, and pass in your AWS Access Key ID, AWS Secret Access Key, Region of us-east-1, and default output format.

#### Linux

If you are using a rolling release distro, e.g. Arch, then you can likely install the newest version using pacman or your distro's package manager. If you are running 20.04 the version in the Ubuntu package lists is fine, otherwise you may need to install the `--classic` version with `snap`.

You can install cppcheck via your package manager, e.g. `sudo apt-get install cppcheck`.

Finally, run `./setup-linux-build-environment.sh` to install the system dependencies needed by the protocol.

#### MacOS

You can install both Cmake and cppcheck via Homebrew with the following command:

```bash
brew install cmake cppcheck
```

You also need to have installed Xcode and installed the Xcode CLI tools, which you can install by running `xcode-select --install` in a terminal after having installed Xcode.

##### Windows

First, you will want to install Cmake with the latest binaries [here](https://cmake.org/download/), and cppcheck with Chocolatey by running `choco install cppcheck --force`. This will ensure you can properly debug the protocol. (Note that `winget` has gotten pretty mature, so you might just do `winget install cmake` and `winget install cppcheck`).

Install the latest version of Ninja as well if you are compiling on Windows (this may come with your Visual Studio, depending on how you configured it).

In order to compile it, you need to first install [Microsoft Visual Studio Community 2019](https://visualstudio.microsoft.com/downloads/) and select `Desktop Development with C++` add-on in the installer. This will install different Visual Studio Command Prompts for different architectures. **In order to compile the protocol, you need to make sure to be using x86_64 Visual Studio Developer Command Prompt.** In order to open the x86_64 Visual Studio Developer Command Prompt, you must First hit the "Windows Key", then you must type in "x86_64", and then the first result should indeed be the x86_64 Visual Studio Developer Command Prompt. **All future steps must be done using this terminal, otherwise compilation will not work**

Note that if you're returning to Windows development after some time, you might have an old version of the Windows 10 SDK installed. In order to build the protocol, open the Visual Studio Installer, click "Modify" on your installation, navigate to "Individual Components", search for "Windows 10 SDK", and ensura that your installed version is at least `10.0.22000.0`. Otherwise, protocol builds will fail due to compiler warnings.

---

NOTE: Ignore the below paragraph. CUDA is not necessary for Windows Development.

If you wish to get CUDA working anyway (Not Recommended), you also need to install the Nvidia CUDA Compiler, NVCC. Note that CUDA is serverside only, you do not need this for the Windows client! You'll need at least [Toolkit version 11.0](https://developer.nvidia.com/cuda-11.0-download-archive?target_os=Windows&target_arch=x86_64&target_version=10&target_type=exenetwork), which is latest at time of writing. If you have an Nvidia GPU, feel free to install all the components if you don't mind overwriting your display driver with whatever comes with the 11.0 release. If you don't have an Nvidia GPU, you have to be careful which components you install. At the Options page of the installer, choose the Custom Install. Don't install the Driver components or the Other components (it might be bad to overwrite your driver if you don't have an Nvidia GPU or you don't want the driver packaged with the 11.0 release). Meanwhile, under CUDA, you MUST install the Development→Compiler and Runtime→Library. Everything else is optional, though you might find the extra tools, samples, or visual studio integration useful.

---

After doing this all this, you might have to restart your terminal or IDE, after which you'll be able to build using CMake.

### Building the Protocol

In order to save time recompiling dependencies repeatedly (and use CI to update them), we store some precompiled libraries in [AWS S3](https://s3.console.aws.amazon.com/s3/buckets/fractal-protocol-shared-libs?region=us-east-1&tab=objects). Downloading them will require the AWS CLI to be installed and set up on your machine. Instructions for installing and configuring awscli can be found above.

If you want to manually retrieve the binaries yourself (Not Recommended), download the files from S3 into the proper locations (see `download-binaries.sh`), and _then_ run `cmake` with the `-D DOWNLOAD_BINARIES=OFF` option. The order of these operations is important.

To compile on MacOS/Linux, run:

```bash
mkdir build
cd build
cmake -S .. -B .
make -j
```

This should be run from the root `/protocol/` directory. This will ensure that all built files end up in the `/build` directory and are easy to clean up. To recompile, just `cd` into the `/build` directory and run `make -j` again.

On Windows,

```powershell
.\cninja.bat
```

The `cninja.bat` script lives in the `/protocol/` directory, but can be called from anywhere.

If you're having trouble compiling, make sure that you installed all the necessary dependencies. If you still have issues, try deleting the CmakeCache or start from a fresh repository.

### Running the Protocol

From the build directory (usually `/build`), you can simply run:

```bash
# Windows:
wclient IP_ADDRESS [OPTION]...
wserver [OPTION]...

# MacOS/Linux:
./wclient IP_ADDRESS [OPTION]...
./wserver [OPTION]...
```

For the specific option flags for the client, see the [client-specific README](./client/README.md). If you want to see the specific option flags for the server, you're straight out of luck.

For a useful workflow for server development on Linux, see the [server-specific README](./server/README.md).

**Important:** When running the protocol, please ensure that the client and server are both running on the exact same git commit. If you run into issues, check `git log` on both client and server to ensure the top git hash matches, and ensure that both sides have been compiled.

#### Sentry

The sentry-native SDK gets automatically built by our CMake build system.

To start the protocol with a given Sentry configuration, use the `-e` argument, for instance with `.\WhistClient.exe -e prod <IP>`. See the output of `WhistClient.exe --help` for more details.

### Tips for Specific Tools

#### IDEs

We use CMake to build. If you are using VS code, VS or Clion, this is pretty easy to use. You need to either open the root repo folder as a project, or open the root `CMakelist.txt` as a project. On CLion and VS there is a menu to build at the top, on VS code you need the CMake extension and the build command is at the bottom. CMake currently has two types of builds, Debug and Release. You probably want to be building debug builds while developing, since they log more aggressively (Warning levels: Info and above).

Currently, we use the same compiler flags for Debug and Release because we distribute binaries with debug flags, to better troubleshoot errors and bugs.

The build target for client is "WhistClient" and the server is "WhistServer".

#### MacOS CLI

You can simply run `cmake -S . -B build` from the root folder, `protocol`, which will generate the makefiles in the `build` directory. You can then run `make WhistClient` from the `build` folder, or cd into `build/client` and run `make` to compile the MacOS client. The client will be in `protocol/build/client/build64`.

#### Linux CLI

A helpful TUI for configuring the build is `ccmake`. To use it, run `ccmake .` in the root of the protocol repo. You will initially see a blank screen because no cache has been built yet. Hit `c` to configure. This will populate the cache and show you a page with various settings. The setting you will likely care about is `CMAKE_BUILD_TYPE` which is one of `Debug` or `Release`.

Next hit `c` again to reconfigure with your possibly new settings, then hit `g` to generate the makefile. This makefile has all of the build targets, including `WhistClient`, `WhistServer` and all of our libraries. It also includes CMake targets to clean, edit, and rebuild the cache.

Only running `make` defaults to building WhistClient and WhistServer if you set both of these to ON in your configuration.
GCC only supports one type of build at a time, so if you are currently building Release, but want to build Debug, you need to edit the cache and regenerate the makefile.

When making `Debug` builds, it is often desirable to run them with sanitizers - these are compiler tools which add extra run-time checks into the code to find common errors and report them. Enable one or more of these with the `SANITIZE` cmake option.

- `address` (ASan): finds memory errors like array overruns and use-after-free, and also reports on memory leaks once the program ends.
- `undefined` (UBSan): finds undefined behaviour like integer overflow and misaligned pointers.
- `thread` (TSan): finds data-races, including unsafe use of mutexes and file descriptors across threads.
- `OFF`: disable all (set to clear a previously-cached value).
- Other options also exist, consult the documentation for the compiler you are using:
  - [gcc instrumentation options](https://gcc.gnu.org/onlinedocs/gcc/Instrumentation-Options.html#index-fsanitize_003daddress).
  - [clang code generation operations](https://clang.llvm.org/docs/UsersManual.html#controlling-code-generation).

These can be combined in the `SANITIZER` option by separating them with `+`, though some combinations (such as TSan and ASan together) are not supported. CI currently runs tests with `address+undefined` set, and will fail if they generate any errors.

### Further documentation

We also use Doxygen in this repository. The Doxy file is `docs/Doxyfile`. To generate it, you should first install `doxygen`, and then run `doxygen Doxyfile`. This will generate the docs and put them in `docs/html` and `docs/latex`. You can view the html docs by opening the index.html page with a web browser. We keep the docs `.gitignore`d to avoid clutter on the repository. They are also published at `docs.whist.com`.

## CI & CD

#### Checks & Tests

These builds will also have `cppcheck` run against them which is a static analysis tool which can catch errors which compilers cannot, e.g. accessing uninitialized memory and other undefined behavior. Not everything it catches is critical, but it does indicate the possibility of unexpected behavior.

These builds will also (TODO) be tested against a live server VM. This workflow will spin up an Azure VM, upload the server build to it, and then use GitHub Actions VMs on Windows, MacOS and Linux Ubuntu as clients to connect and stream via the protocol for one minute. This will also occur nightly against the `dev` branch, but these builds will not be released (this can be removed once testing is stable and re-enabled on all commits).

To see the warnings in context go to the Actions tab, click on your PR/push that launched the action, select an OS it ran on and then select build. This expands the build log, where you can clearly see the warnings/errors generated.

#### Unit Testing

The protocol currently uses gtest to create and execute unit tests for the `protocol` repository.
To add tests for the `protocol` module, add tests to the `protocol_test.cpp` file in the test folder.

To run all unit tests, recompile the protocol by going into the `protocol/build` directory and
running `make -j`. This is assuming you have already ran `cmake -S .. -B .` from the Building the Protocol
section. If you have not done that, do so before running `make -j`.

Next, run `cd test`. Now:

To run the unit tests for the protocol, run `./WhistProtocolTest`

On macOS, `make -j test` will build and run the unit tests.

If only a particular test case needs to be run, then use the below `--gtest_filter` argument like in the example below.

`GTEST_ARGS="--gtest_filter=ProtocolTest.LoggerTest" make test`

### Continuous Integration

For every push or PR, the code will be automatically linted via clang-format according to the [styling](#styling) guidelines.

## Styling

### clang-format

For `.c` and `.h` files, we are formatting using the clang format `{BasedOnStyle: Google, IndentWidth: 4}`. You can download clang-format via the package manager for your respective OS:

```bash
# MacOS:
brew install clang-format

# Linux Ubuntu:
apt-get install clang-format

# Windows:
choco install llvm --force
```

If using vim, VSCode, or Visual Studio, please set this up in your editor to format on save if possible (in Visual Studio, this is through the C/C++ extension settings, as well as the general 'Format on Save' setting/shortcut). Otherwise, please make sure to run your code through `clang` before commits. You can run it from the CLI by running `cmake . && make clang-format`, on MacOS/Linux, or `cmake -G "NMake Makefiles" && make clang-format`, on Windows.

We have [pre-commit hooks](https://pre-commit.com/) with clang-format support installed on this project, which you can initialize by first installing pre-commit via `pip install pre-commit` and then running `pre-commit install` to instantiate the hooks for clang-format.

We also have a custom build target in the CMake 'clang-format' which will run with this style over all `.c` and `.h` files in `server/`, `client/`, and `whist/`. You can call it by running `make clang-format`.

### clang-tidy

Make sure your code is following code standards by running the script `./run-clang-tidy.sh`. The dependencies for each OS are listed below:

```bash
# MacOS:
# set up clang-tidy and clang-apply-replacements
brew install llvm
ln -s "$(brew --prefix llvm)/bin/clang-tidy" "/usr/local/bin/clang-tidy"
ln -s "$(brew --prefix llvm)/bin/clang-apply-replacements" "/usr/local/bin/clang-apply-replacements"

# Linux Ubuntu:
# clang-tidy and clang-apply-replacements-10 should come installed with clang
sudo apt install clang-tidy -y

# Windows:
# Go to https://releases.llvm.org/download.html and install the pre-built LLVM binary. In the installation wizard, make sure to select "add to path".
```

Running `./run-clang-tidy.sh` will find all code standard violations and give you the option to modify and accept or reject changes.
Running './run-clang-tidy.sh -c' will find whether there are any code standard violations and exit with status 1 if there are (designed for CI usage).

If you want any lines to be exempted from the clang-tidy check, either place `// NOLINTNEXTLINE` in the line above or `// NOLINT` at the end of the line.

Macro constants will NOT be checked by this script because of the variety of implementations of `#define`. Please be sure to follow appropriate standards for `#define`s when committing code.

### Logz.io

We currently use Logz.io to store our logs. To get the logs for a session_id, navigate to the `whist/protocol` directory, then type in: `python3 logz-to-text.py <<sesion_id>>` where `<<session_id>>` is the id of the currently running session.

This will output two files:

1. `<<session_id>>-client-<<account>>.txt`: These are the sorted logs for the client from that session_id
2. `<<session_id>>-server-<<account>>.txt`: These are the sorted logs for the server from that session_id

Where `<<account>>` is a logz_io account (prod, dev, staging, base).

In addition, you can choose the download destination via the `LOGS_DOWNLOAD_FOLDER` environment variable.
This determines the destination folder to download the logs. Importantly, this folder is
RELATIVE to the protocol folder you are executing the script from.

If this is not set, it will default to downloading the log files to the protocol directory.

Note: logz.io currently only retains logs for 7 days, as of 10/29/2021. That said, this will only return the logs from 5 days ago. If this changes, please modify the `RETENTION_PERIOD_DAYS` constant in the top of `logz-to-text.py` to reflect this change.
