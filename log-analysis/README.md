# Fractal Log Analysis Tools

This repository contains internal tools used to analyze logs and debug the associated code. If you make any new tools that could be re-used for analyzing and debugging, please add them to this repository.

## Existing Tools
- Memory analysis from protocol logs to find memory leaks
- Latency analysis from protocol logs to find bottlenecks

To use either of these two tools, you need to provide a Fractal protocol log file, either client or server, named `log.txt`. You can retrieve the ones of interest to you from the [Fractal Admin Dashboard](https://fractal-dashboard.netlify.app/). The notebook will then parse the file and plot some statistics from what it finds in the log, which should help with debugging and finding bottlenecks.
