# Fractal GitHub Actions Workflows

This subfolder contains the YAML workflow files for our GitHub Actions workflows. These workflows are integral to our continuous integration, handling everything from tests to building and deployment to cron jobs for cleanup and analysis.

## Syntax

The syntax for workflows is documented in the [GitHub Docs](https://docs.github.com/en/free-pro-team@latest/actions/reference/workflow-syntax-for-github-actions).

## Conventions

### Filenames

Since GitHub does not yet allow us to sort our workflow files into directories, we must name them in a clear and consistent manner. In particular, we name our workflows as `[subproject]-[verb]-[noun].yml`.

For example, a workflow for the `webserver` project which clears protocol logs is named `webserver-clear-protocol-logs.yml`, whereas a meta workflow (one which operates on workflows and PRs themselves) which labels pull requests is named `meta-label-pull-requests.yml`. Workflows that belong to the whole repo -- for example, for pushing Sentry releases, are instead written `fractal-deploy-sentry-releases.yml`.

### Headers

Our workflow files start with a simple comment header with a description of the workflow in complete sentences.

For example, this is the header and beginning of `fractal-deploy-sentry-releases.yml`:

```
# workflows/fractal-push-sentry-releases.yml
#
# Fractal: Push Sentry Release
# Automatically push a new Sentry release for each of the fractal/fractal projects.

name: "Fractal: Push Sentry Releases"
```

Note that in addition to the filename, we entitle our workflow with `Project Name: Project Description in Title Case`, wrapping that title in quotes to escape the YAML syntax. This should always match the third line of the header.

### Jobs

Many of our workflows have a single job, whereas some of our more complex workflows (for example, `fractal-publish-build.yml`) will contain multiple jobs triggered by the same event.

It will be important as we scale for our jobs to be named uniquely so that we can programatically listen for specific jobs. The simplest way to achieve this is to enforce the following naming scheme: jobs will be named as `[subproject]-[verb]-[noun]-[jobname].yml`. Note that for single-project YAML files, this should match `[filename]-[jobname].yml`.

In many cases, `jobname` can simply be `main` -- for example, here is the start of the job description in `protocol-linting.yml`:

```
jobs:
    protocol-linting-main:
        name: Lint Protocol
        runs-on: ubuntu-latest
```

Notice that we also include a `Title Case`, plaintext name for the job, in addition to the tag `protocol-linting-main`.

For more complex workflows, we should be specific both in the programmatic identifier and the plaintext job name. For example, here is the start of the job description for pushing the Host Service to an AWS S3 bucket in `fractal-publish-build.yml`:

```
jobs:
    host-service-publish-build-s3:
        name: "Host Service: Build & Publish to AWS S3"
        runs-on: ubuntu-20.04
```

Something to keep in mind when writing jobs, is that neither `cmd` nor `powershell` will fail if a command it runs fails. So, you should explicitly check if any commands you want to succeed, does indeed succeed. If you attach a job with `shell: bash` without specifying any arguments, then the job will _also_ not fail if any command it runs fails, unless it's the very last command (`cmd`/`powershell` will still not fail, even if the last command fails).

On macOS and Linux Ubuntu GitHub Actions runners, `bash --noprofile --norc -eo pipefail {0}` will be the default shell if you don't specify `shell:`, and so it the run command will fail if _any_ command fails. If you're specifying the `shell:` parameter yourself, make sure to add the relevant `-eo pipefail` arguments to ensure that it fails appropriately. On Windows, `cmd` will be the default shell, and failure needs to be checked for explicitly. For more information, see the [documentation](https://docs.github.com/en/actions/reference/workflow-syntax-for-github-actions#using-a-specific-shell).

### Styling

These YAML files are formatted with [Prettier](https://github.com/prettier/prettier). After installing, you can check formatting with `prettier --check .github/*.{yml,yaml}`, and you can fix formatting with `prettier --write .github/*.{yml,yaml}`. VSCode and other IDEs and text editors have pretty robust Prettier integration, so if you've set that up, that also works well! Note that we use a single `.prettierrc` for all Prettier formatting across our entire monorepo, which is why Prettier must be run from the top-level folder.

## Testing

The easiest way to test a workflow is to enable it to be run manually -- to do this, make sure the repository's default branch version of the workflow contains the trigger

```
on:
   workflow_dispatch:
```

Below this, you can optionally specify input parameters. Then, navigate to the page for the workflow in the Actions tab, where a button should appear allowing you to run the workflow manually.

## Contributing

Since we use `dev` as our head branch, there is usually no reason for workflows to be merged up to `prod` prematurely. Simply create your workflow, test it manually via `workflow_dispatch` or via setting it to run on `push` to your feature branch, and then PR it to `dev` as usual.

## Documentation

When writing a complicated workflow, or when you figure out how a confusing undocumented workflow works, please open a PR to `dev` documenting the details of that workflow below.
