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
  # you can replace --help with arguments like the ones below to test
  # --path config --deploy dev --os win32
```

Or if you have all the dependencies in installed in `requirements.txt`, you can
run directly with:

```sh
python3 .github/actions/monorepo-config/main.py --help
# you can replace --help with arguments like the ones below to test
# --path config --deploy dev --os win32
```


# High-level overview

There are three main components that make up the configuration building program:
1. A Python program (`main.py` and `helpers` in this folder)
2. A Docker environment to install and run the Python program
3. A GitHub Action that deploys the Docker environment in the GitHub runner

The Python program has a CLI built with the `click` library. All options are documented with the `--help` flag. `main.py` is the entrypoint to the program. After `pip install -r requirements.txt`, you can `python main.py --help` to see how run the program.

The inputs to the Python program include a "config" path (config folder in monorepo root), "secrets", and "profiles".

"Secrets" is one or more JSON dictionaries of values that will be passed in during a CI run. This will be merged with the rest of configuration.

"Profiles" refers to one of the sets of keys in "config/profiles.yml". These are used to choose between values like "macos/win32/linux". With a profile of "macos", a config like this:

``` json
{
  "PROTOCOL_FILE_NAME": {
    "macos": "_Fractal",
    "win32": "Fractal.exe",
    "linux": "Fractal"
  }
}
```

...gets flattened to this:

``` json
{
  "PROTOCOL_FILE_NAME": "_Fractal"
}
```

This allows you to pass only the necessary configuration values in your application build. Profiles are optional, and if you don't pass them then your build will receive the original nested data structure.
