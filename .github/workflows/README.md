# Fractal GitHub Actions Workflows

This repository contains the YAML workflow files for our GitHub Actions workflows. These workflows are integral to our continuous integration, handling everything from tests to building and deployment to cron jobs for cleanup and analysis.

## Syntax

The syntax for workflows is documented in the [GitHub Docs](https://docs.github.com/en/free-pro-team@latest/actions/reference/workflow-syntax-for-github-actions).

## Conventions

### Filenames

Since GitHub does not yet allow us to sort our workflow files into directories, we must name them in a clear and consistent manner. In particular, we name our workflows as `[subproject]-[verb]-[noun].yml`.

For example, a workflow for the `main-webserver` project which clears protocol logs is named `main-webserver-clear-protocol-logs.yml`, whereas a meta workflow (one which operates on workflows and PRs themselves) which labels pull requests is named `meta-label-pull-requests.yml`. Workflows that belong to the whole repo -- for example, for pushing sentry releases, are instead written `fractal-push-sentry-releases.yml`.

### Headers

Our workflow files start with a simple comment header with a description of the workflow in complete sentences.

For example, this is the header and beginning of `fractal-push-sentry-releases.yml`:

```
# workflows/fractal-push-sentry-releases.yml
#
# Fractal: Push Sentry Release
# Automatically push a new sentry release for each of the fractal/fractal projects.

name: "Fractal: Push Sentry Releases"
```

Note that in addition to the filename, we entitle our workflow with `Project Name: Project Description in Title Case`, wrapping that title in quotes to escape the YAML syntax. This should always match the third line of the header.

### Jobs

While many of our workflows have a single job, whereas some of our more complex workflows (for example, `fractal-publish-build.yml`) will contain multiple jobs triggered by the smae event.

It will be important as we scale for our jobs to be named uniquely so that we can programatically listen for specific jobs. The simplest way to achieve this is to enforce the following naming scheme: jobs will be named as `[subproject]-[verb]-[noun]-[jobname].yml`. Note that for single-project YAML files, this should match `[filename]-[jobname].yml`.

In many cases, `jobname` can simply be `main` -- for example, here is the start of the job description in `protocol-linting.yml`:

```
jobs:
    protocol-linting-main:
        name: Lint Protocol
        runs-on: ubuntu-latest
```

Notice that we also include a `Title Case`, plaintext name for the job, in addition to the tag `protocol-linting-main`.

For more complex workflows, we should be specific both in the programmatic identifier and the plaintext job name. For example, here is the start of the job description for pushing the ECS host service to an AWS S3 bucket in `fractal-publish-build.yml`:

```
jobs:
    ecs-host-service-publish-build-s3:
        name: "ECS Host Service: Build & Publish to AWS S3"
        runs-on: ubuntu-20.04
```

### Styling

These YAML files are formatted with [Prettier](https://github.com/prettier/prettier). After installing, you can check formatting with `cd .github/workflows && prettier --check .`, and you can fix formatting with `cd .github/workflows && prettier --write .`. VSCode and other IDEs and text editors have pretty robust prettier integration, so if you've set that up, that also works well!

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
