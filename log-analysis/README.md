# Fractal Log Analysis Tools

This repository contains internal tools used to analyze logs for debugging, refining, visualization, and more. If you generate new tools to process logs at Fractal, please add them to this repository. The tools are broken down by the Fractal codebase they related to.

## Webserver

We have one tool for processing `main-webserver` events:

- Datadog Events Analysis from webserver Datadog logs

This tool runs weekly from the GitHub Actions workflow `run-events-analysis.yml` via a cron job, storing logs in AWS S3 and visualizing the result in Datadog

We have a few Python scripts to log important events from the webserver logs to Datadog via storing in S3 and running cron jobs to generate the visualization.
