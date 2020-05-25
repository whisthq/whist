## Fractal Windows/Linux Ubuntu Servers

This folder builds a server to stream via Fractal. It supports Windows and Linux Ubuntu. See below for basic build instructions, and the folder's README for build and development details.

### Development

You can run `server.bat` (Windows) or `server.sh` to restart your dev environment.

To compile, you should first run `cmake .` (MacOS/Linux) or `cmake -G "NMake Makefiles"` (Windows) from the root directory, `/protocol/`. You can then cd into this folder and run `make` (MacOS/Linux) or `nmake` (Windows).

If you're having trouble compiling, make sure that you followed the instructions in the root-level README. If still having issue, you should delete the CmakeCache or start from a fresh repository.

### Running

From this directory, `/server`, you can simply run:

```
Windows
FractalServer

MacOS/Linux
./FractalServer
```

### Updating Fractal Virtual Machines










### Updating

You can update all Windows virtual machines by running `update.bat`, and all Linux Ubuntu virtual machines by running `update.sh`. This action is only allowed from the stable production branch and needs to be coordinated with the client updating.

