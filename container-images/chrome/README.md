# Google Chrome Container Image

This folder contains the container image for the streaming Google Chrome via Fractal. The Dockerfile is based on an Ubuntu 18.04 image and installs all required 

For reference on the bash functions called in the Docker image, refer to: https://github.com/fractalcomputers/setup-scripts

For reference on the Fractal protocol and streaming, refer to: https://github.com/fractalcomputers/protocol

## Running

First, start the container and the Fractal server:

```
docker run -p 5900:5900 --name chrome --user fractal --privileged <image-name>
```

Once the container is running, you can use Fractal to go into it and run Chrome
from a terminal window by running:
```
google-chrome
```

You can also start Google Chrome by right-clicking the Desktop and selecting:
```
Applications > Network > Web Browsing > Google Chrome
```
