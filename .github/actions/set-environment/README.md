# `set-environment`

The custom `set-environment` Action is used to aggregate environment-specific configuration data that may be consumed by Steps in GitHub Actions Workflows.

### Inputs

- `ref`: A git ref. This ref is used to determine the environment in which the Workflow should run. This input's default value is [`github.ref`][0].

### Outputs

- `environment`: The name of the environment in which the Workflow should run. The `environment` output will be `"prod"` if the input ref matches `prod$`, `"staging"` if the input ref matches `staging$`, and `"dev"` otherwise.

- `auth0-domain`: The domain name of the environment-specific Auth0 tenant with which Workflow steps may communicate.

- `auth0-client-id`: The client ID of the GitHub Actions application registered to the environment-specific Auth0 tenant.

- `auth0-client-secret-key`: The name of the GitHub Secret containing the client secret of the GitHub Actions application registered to the environment-specific Auth0 tenant.

## Using the Action

Here is an example of how the Action may be used in a Workflow:

```yaml
name: My Workflow
on: push
jobs:
  my-job:
    name: My Job
    runs-on: ubuntu:20.04
    steps:
      # The repository must be checked out before the custom Action may be used
      # in the Workflow.
      - name: Checkout git repository
        uses: actions/checkout@v2
      - name: Aggegrate environment-specific configuration data
        id: environment
        uses: ./.github/actions/set-environment
      - name: Echo the name of the environment
        run: echo "${{ steps.environment.outputs.environment }}"
```

## Developing the Action

The `set-environment` Action is a [Docker Action][1]. As such, it is easy to run locally. After making changes to the Action's implementation, run the following command to build its executable Docker container:

    $ docker build -t set-environment /path/to/monorepo/.github/actions/set-environment

Specifying `-t set-environment` is optional and only serves to name the built container `set-environment` so it can be run with:

    $ docker run set-environment my-ref-name

See the [`docker build`][0] and [`docker run`][3] documentation for more information about building and running docker containers.

[0]: https://docs.github.com/en/actions/reference/context-and-expression-syntax-for-github-actions#github-context
[1]: https://docs.github.com/en/actions/creating-actions/creating-a-docker-container-action
[2]: https://docs.docker.com/engine/reference/commandline/build/
[3]: https://docs.docker.com/engine/reference/commandline/run/
