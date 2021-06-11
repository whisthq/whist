# Custom GitHub Actions

This directory contains custom GitHub Actions that have been written in order to reduce the length of and code duplication in our GitHub Workflows. Each Action is represented by its own directory containing at least a file called `action.yml`.

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

# What's a GiHhub Action?

In GitHub terminology, [Actions](https://docs.github.com/en/actions/creating-actions/metadata-syntax-for-github-actions) are distinct from [Workflows](https://docs.github.com/en/actions/reference/workflow-syntax-for-github-actions). Their docs do not quite make this distinction clear, so here's a quick overview.

"Actions" are single units of work. They are distinct environments with a single set of inputs and an output. They are stateless and do not have access to GitHub context, like repository information or secrets. They must be passed any data they'll use as an input.

"Workflows" are multi-step processes made up of jobs and steps. Often, a Workflow will employ an Action as one of these jobs. A well-formed Workflow is mostly a composition of Actions, with the purpose of wiring up inputs and outputs between them. Workflows have access to GitHub context, like repository information and secrets, and can pass data from that context to their jobs and steps.

Actions and Workflows are both defined as YAML files stored in `.github/actions` and `.github/workflows`, respectively.

Workflows are very hard to run and test locally. They're parsed and evaluated based on a complex domain-specific language using names of nested YAML keys, string templating, and a GitHub-flavored subset of JavaScript. To supply data to their jobs, they rely on the GitHub-specific context that's only available when running in the CI step. If you've worked with them before, you've probably gone through the clunky commit-push-deploy-wait loop that's necessary to test your work. The awkwardness of this process has led many of us to write complex Bash or Python scripts directly inside the Workflow YAML, so at least some part of it can be tested locally.

Fortunately, Actions put a lot more control in the hands of the developer. They have a much smaller set of configuration options, and only run one process at a time. When creating an Action, you choose from three environments to run your work.

1. Node.js
2. Docker
3. "Composite"

The simplest of these to work with is Docker, and that's what we'll focus on for the rest of this document. Both Node.js and "Composite" start to bring in the complexity of Workflows by adding jobs and steps and lots of GitHub context that becomes impossible to reproduce locally. By selecting a Docker environment, GitHub effectively "hands off" all execution to your Docker container. You have full control over your environment and dependencies. This allows your to work with a familiar toolset while you're developing and testing, with the expectation that the environment will be (almost) entirely the same when you deploy to GitHub.

Our goal here is to be able to comfortably write a program that can be decoupled from GitHub Actions. We want to be able to do the bulk of our work in a familiar environment, writing code that simply accepts data and returns data. Ideally, our program doesn't need to know that it's running in GitHub Actions. In the next sections, we'll configure our Action and Docker container as a GitHub-focused "wrapper" that can host any general-purpose program.

## Setting up a GitHub Action

New Actions should be developed in a new subdirection under `.github/actions`. We'll use a simplified setup for the `monorepo-config` Action as a demonstration. At minimum, a Docker GitHub Action requires two files, one to one to set up the Action and one to setup your Docker container. You must use `action.yml` and `Dockerfile` as the names.

```

├── .github
│   ├── actions
│   │   ├── monorepo-config
│   │   │   ├── action.yml
│   │   │   └── Dockerfile

```

`action.yml` describes the structure, inputs, and outputs of your Action process for GitHub's runner. Since we're using a Docker environment, we only need a minimal amount of information in this file:

```YAML
name: Monorepo Config
description: Build /config folder into .json files.
inputs: # inputs can be passed to an Action from a workflow.
  secrets: # our config program needs a secrets argument to merge with config.
    description: JSON object of Fractal secrets # just for documentation
    required: true # optional arguments are also supported
outputs:
  config: # our output will be available to the workflow with this key
    description: JSON object of Fractal config
runs: # this is where we select the environment for the action.
  using: docker # could also be a node version like 'node12' or 'composite'
  image: Dockerfile # looks for a Dockerfile in the same folder.
```

The Action setup can be kept very minimal. We're really just defining the inputs and outputs, and letting GitHub know we're using Docker. There are just two slighty odd GitHub Actions rules to know:

1. `inputs` can only be made available to your Docker process as environment variables. They'll be capitalized and prefixed with `INPUT_`. In this example, our process will need to accept `secrets` through the variable `INPUT_SECRETS`.
2. `outputs` receives data from your Docker process through stdout, and the data must be printed in this format: `::set-output name=<output name>::<value>`. When we call our process in the next section, we'll `echo` our output into this string, like so: `echo "::set-output name=config::$(<run-process-command>)".`

## Setting up a Dockerfile

Our `Dockerfile` is going to setup all the resources that our program needs to run. While `action.yml` is only relevant for GitHub's Actions runner during deployment, our `Dockerfile` needs to do double duty. We want to write a single `Dockerfile` that we can deploy to GitHub Actions, as well as build and run locally. This will give us a consistent environment to develop in, so we can write a program that runs in the container and needs no knowledge of GitHub's context, or its strange `input` and `output` needs.

## Tips for Actions environment setup

You should `COPY` your Action to the `/root` folder in the Docker container, and then refer to it by absolute path. For example, here's the `monorepo-config` Dockerfile:

```Dockerfile
COPY ./requirements.txt /root/monorepo-config/requirements.txt
RUN pip install -r /root/monorepo-config/requirements.txt
COPY . /root/monorepo-config
ENTRYPOINT ["python", "/root/monorepo-config/main.py", "config"]

```

Here's the tricky part about GitHub... they pull a fast one on you right before the `ENTRYPOINT`. For the first three instructions above (`COPY`, `RUN`, `COPY`), your working directory is same folder as your Dockerfile (here, it's the `monorepo-config` folder). But on `ENTRYPOINT`, your working directory becomes the repository root (which for us is the `fractal` monorepo folder).

This is unusual behavior if you're used to working with your own Dockerfiles. It's also not behavior that you'll see if you build and run your Dockerfile locally. This can make local testing painful if you're using relative paths.

Because of this inconsistency, it's easiest to deal with absolute paths in the container. By copying the Action folder (`monorepo-config`) to an absolute path, we can defend against GitHub's little bait-and-switch.

There's still one more thing to worry about. In the example above, the `ENTRYPOINT` runs the `main.py` script, and passes it an argument: `config`. That's referring to the `config` folder in the root of the monorepo, and you'll notice that we're referencing it as a relative path. So we haven't completely forgetten about GitHub's directory swap, as we're relying on the working directory for `ENTRYPOINT` to be the monorepo root, and not the `monorepo-config` Action folder.

This will cause you some confusion while developing and testing locally. You're probably used to setting your working directory to the same folder of the project you're working on, and it would be tempting to try and `cd` in to the `monorepo-config` folder if you need to make some changes. However, that's going to break a command like the one above, which is has a relative reference to `config`.

That brings us to the last piece of the puzzle... when building/running/testing your Docker container locally, keep your working directory as the monorepo root (`fractal` folder). That will ensure that your context is completely consistent with what will happen on the GitHub deploy. Your build command might look like this:

```sh
[fractal]$ docker build .github/actions/monorepo-config
```

By the way, don't try and get around this by setting `WORKDIR` in your Dockerfile. GitHub explicitly warns against that [in their docs](https://docs.github.com/en/actions/creating-actions/dockerfile-support-for-github-actions#workdir). If you need to reference the full path of the `ENTRYPOINT` working directory, GitHub makes that available in the `GITHUB_WORKSPACE` environment variable.

That was a lot of context, so here's a recap. There's only two steps you need to take to ensure a consistent development environment for GitHub Actions:

1. Use absolute paths everywhere in the Dockerfile, except for arguments to the `ENTRYPOINT` command.
2. When building and running your container locally, keep your working directory at the top-level `fractal` folder.

That's all! Keep to this practice, and you won't need a special runner like `nektos/act` for developing an Action.
