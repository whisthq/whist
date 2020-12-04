# Fractal Github Actions Workflows

This repository contains the YAML workflow files for our Github Actions workflows. These workflows are integral to our continuous integration, handling everything from tests to building and deployment to cron jobs for cleanup and analysis.

## Syntax

The syntax for workflows is documented in the [Github Docs](https://docs.github.com/en/free-pro-team@latest/actions/reference/workflow-syntax-for-github-actions).

## Conventions

### Filenames

Since Github does not yet allow us to sort our workflow files into directories, we must name them in a clear and consistent manner. In particular, we name our workflows as `[subproject]-[verb]-[noun].yml`.

For example, a workflow for the `ecs-host-service` which publishes builds is named `ecs-host-service-publish-builds.yml`, whereas a meta workflow (one which operates on workflows and PRs themselves) which labels pull requests is named `meta-label-pull-requests.yml`.

### Headers

Our workflow files start with a simple comment header with a description of the workflow in complete sentences.

For example, this is the header and beginning of `main-webserver-push-sentry-release.yml`:

```
# workflows/main-webserver-push-sentry-release.yml
#
# Main Webserver: Push Sentry Release
# Automatically push a new sentry release for the main webserver.

name: "Main Webserver: Push Sentry Release"
```

Note that in addition to the filename, we entitle our workflow with `Project Name: Project Description in Title Case`, wrapping that title in quotes to escape the YAML syntax. This should always match the third line of the header.

### Jobs

Most of our workflows have a single job, though there's no reason why they couldn't have more. It will be important as we scale for our jobs to be named uniquely so that we can programatically listen for specific jobs. The simplest way to achieve this is to enforce the following naming scheme: jobs in a file `filename.yml` will be named `filename-jobname`.

In many cases, `jobname` can simply be `main` -- for example, here is the start of the job description in `protocol-linting.yml`:

```
jobs:
    protocol-linting-main:
        name: Lint Protocol
        runs-on: ubuntu-latest
```

Notice that we also include a `Title Case`, plaintext name for the job, in addition to the tag `protocol-linting-main`.

In other cases, we might wish for multiple jobs to eventually reside in the same workflow; for example, we have a job named `container-images-push-images-ecr`, since we could ostensibly extend this to also push to DockerHub or GCP or Azure.

### Styling

These YAML files are formatted with [prettier](https://github.com/prettier/prettier). After installing, you can check formatting with `cd .github/workflows && prettier --check .`, and you can fix formatting with `cd .github/workflows && prettier --write .`. VSCode and other IDEs and text editors have pretty robust prettier integration, so if you've set that up, that also works well!

## Testing

The easiest way to test a workflow is to enable it to be run manually -- to do this, make sure the master branch version of the workflow contains the trigger

```
on:
   workflow_dispatch:
```

Below this, you can optionally specify input parameters. Then, navigate to the page for the workflow in the Actions tab, where a button should appear allowing you to run the workflow manually.

## Contributing

Workflows should almost always be merged to dev, not to master. When a PR is opened, the workflows that are run are those in the new head branch; there is usually no reason for these to be merged up to master prematurely.

Of course, there are significant cases in which this does not apply. If you know what you're doing, go ahead and PR to master; if you don't, please reach out to others to quickly figure things out.

## Documentation

When writing a complicated workflow, or when you figure out how a confusing undocumented workflow works, please open a PR to dev documenting the details of that workflow below.
