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

## What's a GitHub Action?

In GitHub terminology, [Actions](https://docs.github.com/en/actions/creating-actions/metadata-syntax-for-github-actions) are distinct from [Workflows](https://docs.github.com/en/actions/reference/workflow-syntax-for-github-actions). Their docs do not quite make this distinction clear, so here's a quick overview.

**Actions** are single units of work. They are distinct environments with a single set of inputs and an output. They are stateless and do not have access to GitHub context, like repository information or secrets. They must be passed any data they'll use as an input.

**Workflows** are multi-step processes made up of jobs and steps. Often, a Workflow will employ an Action as one of these jobs. A well-formed Workflow is mostly a composition of Actions, with the purpose of wiring up inputs and outputs between them. Workflows have access to GitHub context, like repository information and secrets, and can pass data from that context to jobs, steps, and Actions.

Actions and Workflows are both defined as YAML files stored in `.github/actions` and `.github/workflows`, respectively. The YAML example above shows how a workflow can call Actions with the `uses:` syntax, giving both the example of a offcial GitHub Action (`actions/checkout@v2`) as well as a custom, local action.

Workflows are very hard to run and test locally. They're parsed and evaluated based on a complex domain-specific language using names of nested YAML keys, string templating, and a GitHub-flavored subset of JavaScript. To supply data to their jobs, they rely on the GitHub-specific context that's only available in the GitHub Actions runner. If you've worked with them before, you've probably gone through the clunky commit-push-deploy-wait loop that's necessary to test your work. The awkwardness of this process has led many of us to write complex Bash or Python scripts directly inside the Workflow YAML, so at least some part of it can be tested locally.

Fortunately, Actions put a lot more control in the hands of the developer. They have a much smaller set of configuration options, and only run one process at a time. When creating an Action, you choose from three environments to run your work:

1. Node.js
2. Docker
3. Composite

The simplest of these to work with is Docker, and that's what we'll focus on for the rest of this document. Both Node.js and "Composite" start to bring in the complexity of Workflows by adding jobs and steps and lots of GitHub context that becomes impossible to reproduce locally. By selecting a Docker environment, GitHub effectively "hands off" all execution to your Docker container. You have full control over your environment and dependencies. This allows you to work with a familiar toolset while you're developing and testing, with the expectation that the environment will be the same when you deploy to GitHub.

Our goal here is to be able to comfortably write a program that can be decoupled from GitHub Actions. We want to be able to do the bulk of our work in a familiar environment, writing code that simply accepts data and returns data. Ideally, our program doesn't need to know that it's running in GitHub Actions. In the next sections, we'll configure our Action and Docker container as a GitHub-focused "wrapper" that can host any general-purpose program.

## Setting up a Docker Action

New Actions should be developed in a new subdirectory under `.github/actions`. Note that this folder isn't run automatically by the GitHub Actions runner like the files in `.github/workflows`. When you want to use one of your custom `.github/actions`, you refer to it a Workflow with the `uses:` syntax. Note that `uses:` should refer to folder _containing_ an `action.yml` file, which we'll create below.

We'll use a simplified setup for the `monorepo-config` Action as a demonstration. At minimum, a Docker GitHub Action requires two files, one to set up the Action and one to setup your Docker container. You must use `action.yml` and `Dockerfile` as their names.

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

The Action setup doesn't need to be any complex than this. We're really just defining inputs and outputs, and letting GitHub know we're using Docker. There are just two slighty odd GitHub Actions rules to know:

1. `inputs` are only made available to your Docker process as environment variables. They'll be capitalized and prefixed with `INPUT_`. In this example, our Docker container will be handed our `secrets` input through the environment variable `INPUT_SECRETS`.
2. `outputs` receives data from your Docker process through stdout, and the data must be printed in this format: `::set-output name=<output name>::<value>`. When we call our process in the next section, we'll `echo` our output into this string, like so: `echo "::set-output name=config::$(<run-process-command>)".`

## Setting up a Dockerfile

Our `Dockerfile` is going to setup all the resources that our program needs to run. While `action.yml` is only relevant for GitHub's Actions runner during deployment, our `Dockerfile` needs to do double duty. We want to write a single `Dockerfile` that we can deploy to GitHub Actions, as well as build and run locally. This will give us a consistent environment to develop in. We can write a program that runs in the container and needs no knowledge of GitHub's context, or its strange `input` and `output` needs.

Think of this `Dockerfile` as a "wrapper" that will provide your program with everything it needs. Its job will be to fetch dependencies, make sure files are in the right place. It will also translate the Action `input` into generic arguments that your program will expect, and it will transform your program's result into the `output` syntax that GitHub Actions expects. If this is done correctly, you'll have a completely reproducible environment in which you can run any program at all, without having to modify that program to work with GitHub.

### Our general-purpose Python program

Let's add the remaining files for our `monorepo-config` Action. It's a Python program, so we'll use `main.py` as our entrypoint, a `helpers` folder for some utilities, and a `requirements.txt` to specify its dependencies.

```
├── .github
│   ├── actions
│   │   ├── monorepo-config
│   │   │   ├── action.yml
│   │   │   ├── Dockerfile
│   │   │   ├── helpers
│   │   │   ├── main.py
│   │   │   └── requirements.txt

```

In this example, `main.py` is a basic Python program that does the actual work for our Action. It has a couple dependencies in `requirements.txt` that you need to `pip install` before running. The important thing to recogize here is that `main.py` is an entirely standard Python file. It reads its arguments from the command line, and prints its results to stdout. It has no knowledge of GitHub Action implementation details, it doesn't need any environment variables, and you can run it locally with `python main.py`. These are all characteristics of a general-purpose command line program, and this is what we want to be able to write.

An important part of this job is managing how arguments get passed to our Python program. Here, `main.py` takes two arguments:

1. The path to the `config` folder.
2. An object of "secrets", which is passed through the `--secrets` flag. In the `actions.yml` file above, this is the `secrets` object that our Action receives as one of its `inputs`.

With a working directory of `fractal`, we might call this:

```
python .github/actions/monorepo-config/main.py config \
       --secrets '{"test": "secret"}' \
```

### Dockerfile walkthrough

Let's look at the contents of our `Dockerfile`, the "wrapper" that helps GitHub-ify our general-purpose program. The most important parts of this file are the paths on each line, so here we've spaced out the `Dockerfile` so each line is easy to compare.

```Dockerfile
COPY       ./requirements.txt /root/monorepo-config/requirements.txt
RUN        pip install -r     /root/monorepo-config/requirements.txt
COPY       .                  /root/monorepo-config
ENTRYPOINT python             /root/monorepo-config/main.py config \
                              --secrets $INPUT_SECRETS
```

Each line in detail:

1. First we `COPY` our `requirements.txt` over to the container. Because Docker caches image layers, it's good practice to seperately copy dependency files like `requirements.txt` or `package.json` over on their own.
2. Next we `pip install -r` our `requirements.txt`. This layer will be cached between Docker builds, and we'll only invalidate the cache if `requirements.txt` changes. This saves us from having to re-download all our dependencies every time we change a file in `monorepo-config`.
3. We `COPY` the entire contents of our working directory, `monorepo-config`, over to the container.
4. `ENTRYPOINT` is the command that will run in the container when we`docker run` this image. In this case, we're running the `main.py` file that we copied over in the previous step. We're passing the path of the `config` folder as its first argument, and passing the environment variable `$INPUT_SECRETS` to its `--secrets` flag.

It's a very simple `Dockerfile`, but there's a hidden layer of complexity that can cause a lot of confusion, so we're going to spend some extra time on it. If you read the `Dockerfile` closely, the `ENTRYPOINT` step might cause you to ask "wait a minute, where did this `config` folder come from"?

### Paths in the COPY steps

Here's the tricky part about GitHub Actions... they pull a fast one on you right before the `ENTRYPOINT`. For the first three instructions above (`COPY`, `RUN`, `COPY`), your working directory is same folder as your Dockerfile (here, it's the `monorepo-config` folder). But on `ENTRYPOINT`, your working directory becomes the repository root (which for us is the `fractal` monorepo folder, which contains the `config` folder).

Our `Dockerfile` is defensive against this behavior. Notice that we're referring to _relative_ paths (like `.` and `./requirements.txt`) as the "source" for the `COPY` directives, because we know we at _build time_ that we're in our Dockerfile's working directory. We refer to _absolute_ paths as the "destination", because we can't be sure exactly at _run time_ where our working directory will be.

Don't hesitate to read and write absolute file paths in your Action container. Your container's lifecycle is only as long as your Action's process (in this case, our `main.py`). Other Actions and jobs will run in different containers.

### Paths in the ENTRYPOINT step

In the example above, the `ENTRYPOINT` runs the `main.py` script, and passes it an argument: `config`. That's referring to the `config` folder in the root of the monorepo (`fractal/config`), and you'll notice that we're referencing it as a relative path. `ENTRYPOINT` executes at _run time_, not at _build time_. At _run time_, our working directory becomes the root of our git repository, and we can refer to `config` as a relative path.

Don't try and get around this by setting `WORKDIR` in your Dockerfile. GitHub explicitly warns against that [in their docs](https://docs.github.com/en/actions/creating-actions/dockerfile-support-for-github-actions#workdir). If you need to reference the full path of the `ENTRYPOINT` working directory, GitHub makes that available in the `GITHUB_WORKSPACE` environment variable.

## Setting up your development environment

The whole point of this was to make local development easy. We've discussed the details of what's going on in GitHub's environment, and now we can focus on yours. You're probably used to setting your working directory to the same folder of the project you're working on, and it would be tempting to try and `cd` into the `monorepo-config` to do `docker build` or run `main.py`.

Instead, when developing Actions you should get in the habit of keeping your working directory as the root of the monorepo. That's the environment the GitHub Action runner will have, and we'll avoid getting confused if we stick to that. You can pass the `.github/actions/monorepo-config` path to `docker build` to specify our `Dockerfile`, so that `docker build` isn't looking for a `Dockerfile` in the monorepo.

We're just about at the finish line. We now have a container that's reproducible between our local environment and the GitHub Actions runner. We can work on `main.py` as we normally would with a fast, local feedback loop. We can be confident that it will be run with the correct paths and dependencies inside the GitHub Actions runner.

Let's see some commands that we might use while developing locally. Remember our working directory is the monorepo root.

```sh
# Run python on your host machine. You must have the dependencies installed.
python .github/actions/monorepo-config/main.py "config" \
       --secrets '{"test": "secret"}'

# When building to run locally, you should give your image a tag.
# You'll need to reference it when you run a container
docker build \
       --tag  fractal/actions/monorepo-config \
       --file .github/actions/monorepo-config/Dockerfile

# If you need to debug your image build, it can help to have a
# more verbose output.
docker build \
       --tag      fractal/actions/monorepo-config \
       --progress "plain" \
       --no-cache

# You can also run local tests with your image after you've built it.
# Remember you'll have to act like GitHub here and pass in your Python
# arguments as environment variables, rather than positionally.
docker run \
       --rm \
       --volume   $(pwd):/root \
       --workdir  "/root" \
       --env      INPUT_SECRETS='{"test": "secret"}' \
       fractal/actions/monorepo-config

# It's often useful to run the container with a different entrypoint, like
# a bash shell. Sometimes you need to have a look around and see what's going
# on with the file system.
docker run \
       --rm \
       --tty \
       --interactive \
       --entrypoint  /bin/bash \
       --volume      $(pwd):/root \
       --workdir     "/root" \
       --env         INPUT_SECRETS='{"test": "secret"}' \
       fractal/actions/monorepo-config

# Every so often, you'll still want to run your Action inside GitHub's CI.
# You can set a "workflow_dispatch" trigger on your workflow to manually run it.
#
# If you're working on a project branch, you may still occasionally want to do
# a "commit-push-check" flow to watch your workflow run. You can automate some
# this with GitHub's command line tool, "gh". This example assumes a workflow
# monorepo-config-test.yml:
#
# A command like the one below can set commit, push, trigger your workflow, and
# then open the GitHub runner page in your web browser so you can see the
# results.

git commit -a -m 'testing the workflow' \
&& git push -u \
&& gh workflow run \
      monorepo-config-test.yml \
      --repo fractal/fractal \
&& gh run view --web


```

That's all! Keep to this practice, and you won't need a special runner like `nektos/act` for developing an Action.

## GitHub environment

The entire GitHub environment will be passed automatically to the Docker image. Note the environment is not identical to the environment of GitHub workflows. An example is below.

```json
{
    "ACTIONS_RUNTIME_TOKEN": "*****",
    "ACTIONS_CACHE_URL": "https://artifactcache.actions.githubusercontent.com",
    "CI": "true",
    "HOSTNAME": "4a265cb02253",
    "GITHUB_ENV": "/github/file_commands/set_env_***",
    "PYTHON_PIP_VERSION": "21.1.2",
    "HOME": "/github/home",
    "GITHUB_EVENT_PATH": "/github/workflow/event.json",
    "RUNNER_TEMP": "/home/runner/work/_temp",
    "GITHUB_REPOSITORY_OWNER": "fractal",
    "GITHUB_RETENTION_DAYS": "14",
    "GPG_KEY": "E3FF2839C048B25C084DEBE9B26995E310250568",
    "GITHUB_HEAD_REF": "",
    "GITHUB_GRAPHQL_URL": "https://api.github.com/graphql",
    "GITHUB_API_URL": "https://api.github.com",
    "ACTIONS_RUNTIME_URL": "https://pipelines.actions.githubusercontent.com/**",
    "RUNNER_OS": "Linux",
    "GITHUB_WORKFLOW": "Monorepo Config",
    "INPUT_SECRETS": "neil-secret",
    "PYTHON_GET_PIP_URL": "https://github.com/pypa/get-pip/raw/***",
    "GITHUB_RUN_ID": "917521391",
    "GITHUB_BASE_REF": "",
    "GITHUB_ACTION_REPOSITORY": "",
    "PATH": "/usr/local/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin",
    "RUNNER_TOOL_CACHE": "/opt/hostedtoolcache",
    "GITHUB_ACTION": "self",
    "GITHUB_RUN_NUMBER": "379",
    "LANG": "C.UTF-8",
    "GITHUB_REPOSITORY": "fractal/fractal",
    "GITHUB_ACTION_REF": "",
    "GITHUB_ACTIONS": "true",
    "PYTHON_VERSION": "3.9.5",
    "GITHUB_WORKSPACE": "/github/workspace",
    "GITHUB_JOB": "hello_world_job",
    "GITHUB_SHA": "83ce92a46274109e0351ff94477bf4d6316793ba",
    "GITHUB_ACTOR": "neilyio",
    "GITHUB_REF": "refs/heads/neilyio/config/monorepo-level",
    "GITHUB_PATH": "/github/file_commands/add_path_***",
    "RUNNER_WORKSPACE": "/home/runner/work/fractal",
    "PWD": "/github/workspace",
    "GITHUB_EVENT_NAME": "workflow_dispatch",
    "GITHUB_SERVER_URL": "https://github.com",
    "PYTHON_GET_PIP_SHA256": "********************************8"
}
```
