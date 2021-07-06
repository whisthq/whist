# Monorepo Config Action

This folder contains the code for the GitHub Action for the monorepo configuration builder. The program is written in Python, and deployed to GitHub Actions as a Docker container.

# How to run locally

Run all commands with the monorepo root (`fractal/`) as the working directory.

```sh
docker build \
  --tag fractal/monorepo-config \
  .github/actions/monorepo-config

docker run \
  # remove the container after running
  --rm \
  # set the working directory to the current folder (fractal root)
  --workdir $(pwd) \
  # mount as volume, allows you to work interactively with the container
  --volume $(pwd):$(pwd) \
  # reference the image that we tagged in the build step
  fractal/monorepo-config \
  # the remaining arguments are passed to the Python program
  # run --help to see the options and documentation.
  --help
  # you can replace --help with arguments like the ones below to test:
  # --path config --deploy dev --os win32
```

Or if you have all the dependencies installed from `requirements.txt`, you can
run directly with:

```sh
python3 .github/actions/monorepo-config/main.py --help
# you can replace --help with arguments like the ones below to test:
# --path config --deploy dev --os win32
```

## High-level overview

There are three main components that make up the configuration building program. Each one "wraps" the one before. Here they are in order:

1. A Python program (`main.py` and `helpers` in this folder)
2. A Docker environment to install and run the Python program
3. A GitHub Action that deploys the Docker environment in the GitHub runner

### Python

The Python program has a CLI built with the `click` library. All options are documented with the `--help` flag. `main.py` is the entrypoint to the program. After `pip install -r requirements.txt`, you can `python main.py --help` to see how to run the program.

The inputs to the Python program include a "config" path (config folder in monorepo root), "secrets", and "profiles".

"Secrets" are one or more JSON dictionaries of values that will be passed in during a CI run. These will be merged with the rest of the configuration.

"Profiles" refers to one of the sets of keys in `config/profiles.yml`. These are used to choose between values like `macos/win32/linux`. With a profile of `macos`, a config like this:

```json
{
  "PROTOCOL_FILE_NAME": {
    "macos": "_Fractal",
    "win32": "Fractal.exe",
    "linux": "Fractal"
  }
}
```

...gets flattened to this:

```json
{
  "PROTOCOL_FILE_NAME": "_Fractal"
}
```

This allows you to pass only the necessary configuration values in your application build. Profiles are optional, and if you don't pass them then your build will receive the original nested data structure.

### Docker

The Dockerfile for this program is small, but it's carefully constructed to allow for a consistent development environment that matches the GitHub Actions runtime.

It's important to know that in GitHub, the Docker "build-time" commands will be run from a working directory relative to the Dockerfile (this folder). At "run-time" (the `ENTRYPOINT` step), the working directory will change to the monorepo root (`fractal/`). It's best to run commands with a working directory as the monorepo root during development.

To run tests or enter a shell inside the container, override the `--entrypoint` flag to `docker run`. Example:

```sh
# run tests onces
docker run \
    --rm \
    --workdir $(pwd) \
    --volume $(pwd):$(pwd) \
    --entrypoint pytest \
    fractal/monorepo-config \
    .github/actions/monorepo-config

# run tests and reload on change
docker run \
    --rm \
    --tty \
    --interactive \
    --workdir $(pwd) \
    --volume $(pwd):$(pwd) \
    --entrypoint pytest-watch \
    fractal/monorepo-config \
    .github/actions/monorepo-config
    --exitfirst \
    --failed-first \
    --new-first \
    --showlocals \
```

These are long commands, so don't try to memorize them. Plug them into your favorite task runner or make an alias. Don't forget to first build the image with the command at the top.

Because we're mounting our repo with `--volume`, we don't need to rebuild the image on every code change. We only need to rebuild the image if we invalidate our Dockerfile, such as when we install new dependencies.

### GitHub

The Docker container running our Python process is deployed in GitHub CI through an Action. This is defined in `action.yml`, and can be called from a CI workflow with the `uses:` syntax.

Because of the limitations of GitHub's Action API, we need to make a couple allowances in `action.yml`. In that file, we define some `inputs` and `outputs` to take in data from a workflow and send back our result. You must give each of the `inputs` a unique name, which poses a problem for us as our CLI allows multiple `--secrets` values. Allowing multiple `--secrets` allows us to merge secrets from different providers, like GitHub and Heroku. So instead of passing `secrets` as the Python program does, the Action `inputs` are `secrets-github` and `secrets-heroku`. We'll have to add new inputs as we add secrets providers.

Another quirk of GitHub Actions is how you communicate between processes. You must `echo` into a string with a strange `"::set-output name=var-name::$(my-data)"` syntax. We don't want this to clog our console output when we're developing, and fortunately GitHub allows a `post-entrypoint` script to run after our standard `entrypoint` finishes.

When you're running locally with `docker run`, you'll execute the Python program inside `entrypoint.sh`. `post-entrypoint.sh` will not run at all locally, it will only run inside the GitHub Actions runner in CI. As the odd output formatting is happening in `post-entrypoint.sh`, we don't need to look at it while running locally.

At the bottom of the `action.yml`, we define some `args` that will be passed to the Python program. This list of arguments will be passed as is through the `Dockerfile` and into `entrypoint.sh`, and all arguments will be received by `python main.py`.
