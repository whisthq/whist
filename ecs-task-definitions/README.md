# Fractal ECS Task Definitions

This subfolder contains the code to generate ECS task definitions for each of the applications we stream in containers on AWS Elastic Container Service (ECS). On ECS, each container image needs an associated task definition, which is why we need a separate ECS task definition for each application we containerize.

## Generating Task Definitions

You can generate a task definition for a specific application from `fractal-taskdef-template.json` by running `./generate_taskdefs.sh [APP]`. For example, `./generate_taskdefs.sh chrome` will generate a task definition to be associated with a Chrome container for running on ECS, with the standard Fractal ECS task definition parameters.

This script assumes that the application you generate a task definition for does not require any specific parameters that are application-specific to be set in the task definition JSON. If it does, you will need to modify `generate_taskdefs.sh` script to do more than simply set the name of the task definition family.

## Design Decisions

Note that the `:rshared` string in the cloud storage mount point is an instance of us using this undocumented hack in AWS: [https://github.com/aws/containers-roadmap/issues/362](https://github.com/aws/containers-roadmap/issues/362). However, it's been around since 2017, so hopefully it doesn't get patched out before we transition to multicloud with Kubernetes/running Docker containers ourselves to bypass the ECS task definitions.

For an explanation of the `%%FRACTAL_ID_PLACEHOLDER%%`s, refer to [the README for `fractal/ecs-agent`](https://github.com/fractal/ecs-agent). In summary, the string `%%FRACTAL_ID_PLACEHOLDER%%` gets replaced at container creation with a randomly-generated string, called the FractalID, by the ECS agent/host service. This `FractalID` is used internally to dynamically and _securely_ create resources like `uinput` devices and cloud storage directories.

## Publishing

Task definitions for all currently supported apps get automatically published to AWS ECS through the `fractal-publish-build.yml` GitHub Actions workflow, under the `ecs-task-definitions-deploy-task-definitions-ecs` job. See `.github/workflows/fractal-publish-build.yml` for the exact list of applications and AWS regions supported.
