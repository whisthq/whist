## Fractal Desktop Clients

This folder builds a client to receive a server stream via Fractal. It supports Windows, MacOS and Linux Ubuntu. See below for basic build instructions.

### Development

You can run `desktop.bat` (Windows) or `desktop.sh` to restart your dev environment.

To compile, you should first run `cmake .` (MacOS/Linux) or `cmake -G "NMake Makefiles"` (Windows) from the root directory, `/protocol/`. You can then cd into this folder and run `make` (MacOS/Linux) or `nmake` (Windows).

If you're having trouble compiling, make sure that you followed the instructions in the root-level README. If still having issue, you should delete the CmakeCache or start from a fresh repository.

### Running

From the `/desktop` directory, you can simply run:

```
Windows:
desktop [IP ADDRESS] [[OPTIONAL] WIDTH] [[OPTIONAL] HEIGHT] [[OPTIONAL] MAX BITRATE]

MacOS/Linux:
./desktop [IP ADDRESS] [[OPTIONAL] WIDTH] [[OPTIONAL] HEIGHT] [[OPTIONAL] MAX BITRATE]
```

Alternatively, if you want to run the executable directly, in the `/desktop/build64` directory or `/desktop/build32`, run:

```
Windows:
FractalClient [IP ADDRESS] [[OPTIONAL] WIDTH] [[OPTIONAL] HEIGHT] [[OPTIONAL] MAX BITRATE]

MacOS/Linux:
./FractalClient [IP ADDRESS] [[OPTIONAL] WIDTH] [[OPTIONAL] HEIGHT] [[OPTIONAL] MAX BITRATE]
```
