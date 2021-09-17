## Fractal Desktop Clients

This folder builds a client to receive a server stream via Fractal. It supports Windows, MacOS, and Linux Ubuntu.

### Option Flags

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

To test if a build of the client runs (by calling the dummy IP address `0.0.0.0`), call:

```bash
./fclient 0.0.0.0 --width 800 --height 600
```

and check the terminal output.

To run the client to connect to a server, first start a host server instance by following the instructions in the `README` in the `host-setup` folder. Once you have the Fractal Host Service running on the server, as well as a Fractal container image (e.g. `base`), follow the instruction listed in the printout in the _host_ terminal that's running the docker container to connect. Open a terminal window on the _client_ machine, and run `fclient` followed by the IP address of your Fractal protocol server, plus some additional flags, according to the instructions on the host terminal printout. You can also add any of the flags from the list above, if desired.

Once a connection has been established with the server, a new window should open on your client machine, and you should see an animated Fractal logo before the Fractal application (e.g. Base or Fractalized Chrome) starts.


