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

To inspect, open `graph.svg` in the browser! It is interactive, so try clicking on functions in the flamegraph to zoom in or using the search functionality in the top right.

Note that in order for function symbols to properly be rendered, you may need to build the protocol in debug mode, using something like

```bash
cmake -S . -B build-debug -D CMAKE_BUILD_TYPE=Debug
cd build-debug
make -j
```

By default, `dtrace` is disabled on macOS -- mac users will need to disable system integrity protections for `dtrace` specifically. If you have trouble with the [online guide](https://poweruser.blog/using-dtrace-with-sip-enabled-3826a352e64bj) for doing this, then please reach out to @rpadaki who can help you to configure your system.

### Perf Flamegraph

The `perf` flamegraph is especially useful on the server side, but some setup is needed. Since the server protocol runs inside a mandelbox, we must follow the [guidelines below](#mandelbox-considerations). In particular, you will need to profile the mandelbox from the EC2 host, rather than working directly inside the mandelbox.

When `perf` generates profiling data, it expects to locate the binary at the command path associated with the process _inside the mandelbox_. Therefore, you will need to use symlinks to fake the mandelbox binary location from the host. To do this, simply run on the host:

```bash
mkdir -p /usr/share/whist/bin
sudo ln -sf /home/ubuntu/whist/protocol/build-docker/server/build64 /usr/share/whist/bin
```

Note that for the client side of the end-to-end testing framework, the above command is modified to match the client protocol. If you have cloned the monorepo to a different location in your instance, please modify the above command appropriately.

If you want more detailed profiling into some of our dependencies, make sure that the relevant packages are installed on the host. For example, for ALSA shared objects to be installed in the correct location, you may need to run `apt install libasound2-plugins` on the host.

And remember that in order for function symbols to properly be recorded, build the protocol in debug mode (which is the default for mandelbox builds).

Once the setup is complete, the execution is once again simple! Run `pidof WhistServer` to get the PID, followed by

```bash
sudo ./perf-flamegraph.sh $PID`
```

As with the DTrace flamegraph, an interactive `graph.svg` will be generated.

### Other Flamegraphs

Flamegraphs are a very powerful tool and can be used with a number of other profilers. Feel free to add any more useful scripts you build! Again, see https://github.com/brendangregg/FlameGraph for utilities and inspiration.

## Mandelbox Considerations

Due to `seccomp` and `apparmor` filters, as well as namespace switching performed by Docker, directly using many profiling tools inside mandelboxes may be inadequate or impossible. In these cases, it may be desirable to profile _from the host_.

For an example with `gdb`, one can run from the host:

```bash
sudo gdb -p $(pidof /usr/share/whist/WhistServer)
```
