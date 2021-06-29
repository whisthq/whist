# Fractal ECS Agent

**This directory was originally forked from [aws/amazon-ecs-agent](https://github.com/aws/amazon-ecs-agent) to [fractal/ecs-agent](https://github.com/fractal/ecs-agent), and that repo was then copied into this directory of the host service.**

We forked the original `ecs-agent` package to be able to better modify it and integrate it into the host service. The necessary modifications, for now, are in how we actually run the Docker commands to start containers. In particular, we would like to pass in specific directories and device files on container startup for improved security, as well as increased tenability for multi-cloud.

## Building and "Publishing"

**NOTE: If you update the `VERSION` file in this repo, make sure to run `cd ./agent/version && go run gen/version-gen.go` so that the version is actually updated in the auto-generated code!**

## Fractal Changelog

We have added functionality that allows us to, in our task definitions, create and mount a container-specific directory. To do this, we add a bind mount in the taskdef as follows:

```
    {
      "fsxWindowsFileServerVolumeConfiguration": null,
      "efsVolumeConfiguration": null,
      "name": "test-container-specific-dir",
      "host": {
        "sourcePath": "/test/%%FRACTAL_CONTAINER_SPECIFIC_DIR_PLACEHOLDER%%"
      },
      "dockerVolumeConfiguration": null
    }
```

The string `%%FRACTAL_ID_PLACEHOLDER%%` gets replaced with a randomly-generated string, called the `FractalID`, at runtime by the ECS agent (note that this string is not the same as the Docker container ID --- this is because that would require forking the Docker daemon as well, which we can avoid with this workaround. Also, Docker deprecated adding a hostConfig to a container at start-time, which indicates that they might be moving in a direction that would break an implementation of making the `FractalID` equal to the Docker container ID).

We also use the `FractalID` for assigning specific `uinput` devices to each container.

**Note that we probably don't want to become too dependent on AWS' ECS features while using such a modified version of the ECS agent.** In particular, we can't use [docker volumes]( (https://docs.aws.amazon.com/AmazonECS/latest/developerguide/docker-volumes.html) (note that these are slightly different from [bind mounts](https://docs.aws.amazon.com/AmazonECS/latest/developerguide/bind-mounts.html), which are what we use) with the `FRACTAL_ID_PLACEHOLDER`-replacing functionality and expect it to work (It could be done, but it's not currently implemented, since we don't use docker volumes).

Similarly, we don't want to use the new ECS feature of "exec-enabled containers", which has been added to the ECS agent codebase in version 1.50 upstream, since they are created and started using a different codepath (which we have not intercepted) than normal containers.

## License

The Amazon ECS Container Agent is licensed under the Apache 2.0 License.
