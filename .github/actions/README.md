# Custom GitHub Actions

This directory contains custom GitHub Actions that have been written in order to reduce the length of and code duplication in our GitHub workflows. Each Action is represented by its own directory containing at least a file called `action.yml`.

The syntax of the `action.yml` file is documented [here](https://docs.github.com/en/actions/creating-actions/metadata-syntax-for-github-actions).

Note that in order to call a custom Action in a workflow, the repository must be checked out (using the `actions/checkout` action) first. For example:

```yaml
name: My Workflow
on: push
jobs:
  call-my-action:
    name: Call my custom GitHub Action
    runs-on: ubuntu-latest
    steps:
      - name: Checkout the repository
        uses: actions/checkout@v2
        with:
          ref: ${{ github.ref }} # Use this input to specify the exact ref to checkout

      - name: Call my Action
        uses: ./.github/actions/my-action
```

When writing custom Actions, please adhere to the style guidelines described in `.github/workflows/README.md`.

# Tips for Actions environment setup

You should `COPY` your Action to the `/root` folder in the Docker container, and then refer to it by absolute path. For example, here's the `monorepo-config` Dockerfile:

```Dockerfile
COPY ./requirements.txt /root/monorepo-config/requirements.txt
RUN pip install -r /root/monorepo-config/requirements.txt
COPY . /root/monorepo-config
ENTRYPOINT ["python", "/root/monorepo-config/main.py", "config"]

```

Here's the tricky part about GitHub... they pull a fast one on you right before the `ENTRYPOINT`. For the first three instructions above (`COPY`, `RUN`, `COPY`), your working directory is same folder as your Dockerfile (here, it's the `monorepo-config` folder). But on `ENTRYPOINT`, your working directory becomes the repository root (which for us is the `fractal` monorepo folder).

This is unusual behavior if you're used to working with your own Dockerfiles. It's also not behavior that you'll see if you build and run your Dockerfile locally. This can make local testing painful if you're using relative paths.

Because of this inconsistency, it's easiest to deal with *absolute* paths in the container. By copying the Action folder (`monorepo-config`) to an absolute path, we can defend against GitHub's little bait-and-switch.

There's still one more thing to worry about. In the example above, the `ENTRYPOINT` runs the `main.py` script, and passes it an argument: `config`. That's referring to the `config` folder in the root of the monorepo, and you'll notice that we're referencing it as a relative path. So we haven't completely forgetten about GitHub's directory swap, as we're relying on the working directory for `ENTRYPOINT` to be the monorepo root, and not the `monorepo-config` Action folder.

This will cause you some confusion while developing and testing locally. You're probably used to setting your working directory to the same folder of the project you're working on, and it would be tempting to try and `cd` in to the `monorepo-config` folder if you need to make some changes. However, that's going to break a command like the one above, which has a relative reference to `config`.

That brings us to the last piece of the puzzle... when building/running/testing your Docker container locally, keep your working directory as the monorepo root (`fractal` folder). That will ensure that your context is completely consistent with what will happen on the GitHub deploy. Your build command might look like this:

```sh
[fractal]$ docker build .github/actions/monorepo-config
```

By the way, don't try and get around this by setting `WORKDIR` in your Dockerfile. GitHub explicitly warns against that [in their docs](https://docs.github.com/en/actions/creating-actions/dockerfile-support-for-github-actions#workdir). If you need to reference the full path of the `ENTRYPOINT` working directory, GitHub makes that available in the `GITHUB_WORKSPACE` environment variable.

That was a lot of context, so here's a recap. There's only two steps you need to take to ensure a consistent development environment for GitHub Actions:

1. Use absolute paths everywhere in the Dockerfile, except for arguments to the `ENTRYPOINT` command.
2. When building and running your container locally, keep your working directory at the top-level `fractal` folder.

That's all! Keep to this practice, and you won't need a special runner like `nektos/act` for developing an Action.
