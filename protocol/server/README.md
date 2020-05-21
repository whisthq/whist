## Fractal Windows/Linux Ubuntu Server

This code turns a Windows or Linux Ubuntu computer into a server listening to receive a client connection to stream its desktop to at ultra-low latency over the Internet.

### Development

You can run `server.bat` (Windows) or `server.sh` to restart your dev environment.

### Running

```FractalServer```

### Updating

You can update all Windows virtual machines by running `update.bat`, and all Linux Ubuntu virtual machines by running `update.sh`. This action is only allowed from the stable production branch and needs to be coordinated with the client updating.

### Building (depreciated)
It should be easily built via `make` (on Linux Ubuntu with gcc) or `nmake` (on Windows with MSVC - requires Visual Studio Desktop Development Extension). Tested with `Visual Studio Community Edition 2019`.

