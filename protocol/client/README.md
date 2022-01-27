## Whist Desktop Clients

This folder builds a client to receive a server stream via Whist. It supports Windows, MacOS, and Linux Ubuntu.

### Option Flags

The option flags are as follows:

```
  -w, --width=WIDTH             Set the width for the windowed-mode
                                  window, if both width and height
                                  are specified
  -h, --height=HEIGHT           Set the height for the windowed-mode
                                  window, if both width and height
                                  are specified
  -b, --bitrate=BITRATE         Set the maximum bitrate to use
  -c, --codec=CODEC             Launch the protocol using the codec
                                  specified: h264 (default) or h265
  -k, --private-key=PK          Pass in the RSA Private Key as a
                                  hexadecimal string
  -u, --user=EMAIL              Tell Whist the user's email. Default: None
  -e, --environment=ENV         The environment the protocol is running in,
                                  e.g production, staging, development. Default: none
  -i, --icon=PNG_FILE           Set the protocol window icon from a 64x64 pixel png file
  -p, --ports=PORTS             Pass in custom port:port mappings, period-separated.
                                  Default: identity mapping
  -n, --name=NAME               Set the window title. Default: Whist
  -r, --read-pipe               Read arguments from stdin until EOF. Don't need to pass
                                  in IP if using this argument and passing with arg `ip`
  -s, --skip-taskbar            Launch the protocol without displaying an icon
                                  in the taskbar
  -x, --new-tab-url             URL to open in new tab
  --help     Display this help and exit
  --version  Output version information and exit
```

To test if a build of the client runs (by calling the dummy IP address `0.0.0.0`), call:

```bash
./wclient 0.0.0.0 --width 800 --height 600
```

and check the terminal output.

To run the client to connect to a server, first start a host server instance by following the instructions in the `README` in the `host-setup` folder. Once you have the Whist Host Service running on the server, as well as a Whist container image (e.g. `base`), follow the instruction listed in the printout in the _host_ terminal that's running the docker container to connect. Open a terminal window on the _client_ machine, and run `wclient` followed by the IP address of your Whist protocol server, plus some additional flags, according to the instructions on the host terminal printout. You can also add any of the flags from the list above, if desired.

Once a connection has been established with the server, a new window should open on your client machine, and you should see an animated Whist logo before the Whist application (e.g. Base or Google Chrome) starts.
