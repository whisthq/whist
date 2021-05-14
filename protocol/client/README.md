## Fractal Desktop Clients

This folder builds a client to receive a server stream via Fractal. It supports Windows, MacOS and Linux Ubuntu. See below for basic build instructions.

### Development

To compile on MacOS/Linux, run:

```
mkdir build
cd build
cmake -S .. -B .
make -j
```

This should be run from the root `/protocol/` directory. This will ensure that all built files end up in the `/build` directory and are easy to clean up. To recompile, just `cd` into the `/build` directory and run `make -j` again.

On Windows,

```
.\cninja.bat
```

The `cninja.bat` script lives in the `/protocol/` directory, but can be called from anywhere.

If you're having trouble compiling, make sure that you followed the instructions in the root-level README. If still having issues, you should delete the CmakeCache or start from a fresh repository.

### Running

From the `/build` directory, you can simply run:

```
Windows:
fclient IP_ADDRESS [OPTION]...

MacOS/Linux:
./fclient IP_ADDRESS [OPTION]...
```

The option flags are as follows:

```
  -w, --width=WIDTH             set the width for the windowed-mode
                                  window, if both width and height
                                  are specified
  -h, --height=HEIGHT           set the height for the windowed-mode
                                  window, if both width and height
                                  are specified
  -b, --bitrate=BITRATE         set the maximum bitrate to use
  -s, --spectate                launch the protocol as a spectator
  -c, --codec=CODEC             launch the protocol using the codec
                                  specified: h264 (default) or h265
  -u, --user                    Tell fractal the users email, optional defaults to None"
  -e, --environment             The environment the protocol is running
                                in. e.g prod, staging, dev. This is used
                                for sentry. Optional defaults to dev
  --help     display this help and exit"
  --version  output version information and exit;
```

For example, to run the protocol on IP address `0.0.0.0` in an `800x600` window on Linux, call:

```
./fclient 0.0.0.0 --width 800 --height 600
```
