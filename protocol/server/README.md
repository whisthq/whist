## Fractal Windows/Linux Ubuntu Servers

This folder builds a server to stream via Fractal. It supports Windows and Linux Ubuntu.

### Recommended Workflow for Building Server Protocol on Linux

If you're testing the protocol server on Linux, you're going to need to run it inside a [container image](https://github.com/fractal/fractal/tree/dev/mandelbox-images). The default way to do this is to run `mandelbox-images/build_container_image.sh` followed by `mandelbox-images/run_local_container_image.sh [IMAGE]` on a dev EC2 instance. Then, you would start the client protocol on your local machine, passing in the IP address and port bindings of your remote container (which are output by `run_local_container_image.sh`).

However, note that running `mandelbox-images/build_container_image.sh` to rebuild a new container image every time you rebuild the protocol is really time-consuming and a great way to run out of disk space quickly. Instead, we recommend rebuilding the protocol as usual, and then just running `mandelbox-images/run_local_container_image.sh [IMAGE] --update-protocol`. This copies in a new build of the protocol server into an already-built container as it's starting up, shaving a lot of time off the server protocol development cycle.
