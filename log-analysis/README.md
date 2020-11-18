# Fractal Log Analysis Tools

This repository contains internal tools used to analyze logs for debugging, refining, visualization, and more. If you generate new tools to process logs at Fractal, please add them to this repository. The tools are broken down by the Fractal codebase they related to.

## [Protocol](#protocol)

We have two tools for processing protocol logs, both meant for debugging and finding bottlenecks:

- Memory analysis from protocol logs to find memory leaks 
- Latency analysis from protocol logs to find bottlenecks

To use either of these two tools, you need to provide a Fractal protocol log file, either client or server, named `log.txt`. The notebook will then parse the file and plot some statistics from what it finds in the log, which should help with debugging and finding bottlenecks.

## [Webserver](#webserver)

We have a few Python scripts to log important events from the webserver logs to Datadog via storing in S3 and running cron jobs to generate the visualization.
