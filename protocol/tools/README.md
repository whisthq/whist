# Profiling Tools

This folder contains scripts we can use to profile protocol performance.

## Flamegraphs

Flamegraphs are a useful visualization of hierarchical data. They can be very useful for displaying folder sizes or filtering processes. In our case, we are using them to visualize CPU time spent in various function calls, using the stack to keep track of nested calls. The code for flamegraph generation here comes from Brendan Gregg's excellent scripts, which can be found at https://github.com/brendangregg/FlameGraph.

### DTrace Flamegraph

To generate an interactive flamegraph svg from a snapshot of a protocol run, first launch an instance of the protocol and get the PID using something like:

```bash
pidof WhistClient
```

Next, run: `./dtrace-flamegraph.sh $PID`, which will use `dtrace` to profile the CPU time of the next 60 seconds of the protocol run and output to `graph.svg`.

To inspect, open `graph.svg` in the browser! It is interacrtive, so try clicking on functions in the flamegraph to zoom in or using the search functionality in the top right.

Note that in order for function symbols to properly be rendered, you may need to build the protocol in debug mode, using something like

```bash
cmake -S . -B build-debug -D CMAKE_BUILD_TYPE=Debug
cd build-debug
make -j
```

By default, `dtrace` is disabled on macOS -- mac users will need to disable system integrity protections for `dtrace` specifically. If you have trouble with the [online guide](https://poweruser.blog/using-dtrace-with-sip-enabled-3826a352e64bj) for doing this, then please reach out to @rpadaki who can help you to configure your system.

### Other Flamegraphs

We can also use flamegraphs with other profiling tools such as `perf`, probably by making slight modifications to the `dtrace-flamegraph.sh` script. If you end up doing something like this, please be sure to commit your new script to this folder!

## Mandelbox Considerations

Due to `seccomp` and `apparmor` filters, as well as namespace switching performed by Docker, directly using many profiling tools inside mandelboxes may be inadequate or impossible. In these cases, it may be desirable to profile _from the host_.

For an example with `gdb`, one can run from the host:

```bash
sudo gdb -p $(pidof /usr/share/whist/WhistServer)
```

Note that since the `WhistServer` command is inside the mandelbox, we use the mandelbox path in the `pidof` call.

For tools like `stackcollapse-perf.pl` which expect to be able to locate the binary at the command path associated with the process, you will need to use symlinks to fake the mandelbox binary location from the host. In particular, you may need to run on the host:

```bash
mkdir -p /usr/share/whist
sudo ln -sf /home/ubuntu/whist/protocol/build-docker/client/build64 /usr/share/whist
```
