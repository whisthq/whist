# ECS Task Definitions

This folder contains the code to generate ECS task definitions for each of the applications we stream in containers on AWS Elastic Container Service. On ECS, each container image needs an associated task definition, which is why we need an ECS task definition for each application we containerize.

## Generating Task Definitions

You can generate a task definition for a specific application from `fractal-taskdef-template.json` by running `./generate_taskdefs.sh [APP]`. For example, `./generate_taskdefs.sh chrome` will generate a task definition for running a Chrome container on ECS.

This script assumes that the application you generate a task definition for does not require any specific parameters that are application-specific to be set in the task definition JSON. If it does, you will need to modify `generate_taskdefs.sh`. 

## Design Decisions

Note that the `:rshared` string in the cloud storage mount point is an instance of us using this undocumented hack in AWS: [https://github.com/aws/containers-roadmap/issues/362](https://github.com/aws/containers-roadmap/issues/362). However, it's been around since 2017, so hopefully it doesn't get patched out before we transition to multicloud with Kubernetes/running Docker containers ourselves to bypass the ECS task definitions.

## Publishing

Task definitions get automatically published to AWS ECS through the `build-and-publish.yml` GitHub Actions workflow for all applications specified in the YAML workflow to all AWS regions specified in the YAML workflow.
