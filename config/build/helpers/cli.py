# This file creates the CLI for the configuration program. It's designed
# to have as little knowledge as possible about what goes on in the
# configuration program, which is why its import list is so small.
# This file only knows about the CLI considerations, and that we're expecting
# input and output as JSON.
import json
import click
import toolz
import platform
from pathlib import Path


def parse_cli_profiles(_ctx, _param, args):
    """A helper callback that converts a list of --key value pairs
    into a dictionary. Example:

    args = ["--os", "macos", "--env", "prod"]
           ... returns => {"os": "macos", "env": "prod"}
    """
    result = {}
    for chunk in toolz.partition(2, args):
        if not chunk[0].startswith("--"):
            raise click.BadParameter(
                f"All options must start with --, received: {chunk[0]}"
            )
        if not len(chunk) == 2:
            raise click.BadParameter(f"Missing value for option: {chunk[0]}")
        key, value = chunk
        prefix = len("--")
        result[key[prefix:]] = value if value != "" else None
    if not result:
        return None
    return result


def _coerce_json(_ctx, _param, values):
    """A helper callback for a click-based CLI decorator. Converts a JSON
    object into a Python dictionary, while performing some basic validation.
    The JSON must evaluate into a dictionary, or an error will be raised."""
    if not values:
        return None
    try:
        dicts = [json.loads(v) for v in values]
        if not all(isinstance(d, dict) for d in dicts):
            raise click.BadParameter(
                "All --secrets much be JSON dictionaries."
            )
        return toolz.merge(*dicts)
    except json.JSONDecodeError as err:
        raise click.BadParameter(f"{type(err).__name__}: {err.args[0]}")


def create_cli(main_fn):
    """A utility function to create a CLI that passes configured arguments
    to a main_fn.

    The CLI library immediately parses system arguments and runs
    the main_fn. Custom errors will be generated if any validation fails.

    The only positional argument accepted is the path to the config folder.
    All other arguments are keyword arguments, and are optional.

    The CLI accepts all the keywords that are passed to main_fn, with
    an additional --out parameter. This --out parameter accepts a file
    path, and if passed the output of main_fn will be written to the
    file as JSON. If --out is not passed, we print to stdout.

    Args:
        main_fn: A function that accepts the keyword arguments:
                 -- secrets:    A dictionary of secrets to be merged with the
                                parsed configuration schemas.
                 -- profiles:   A list of keys to use in flattening the
                                configuration schemas.
    Returns:
        None
    """

    @click.command(context_settings=dict(ignore_unknown_options=True))
    @click.option(
        "--path",
        default=Path(__file__).joinpath("../../../").resolve(),
        type=click.Path(exists=True),
        help=(
            "Directory containing 'schema/' and 'profile.yml'"
            + ", defaults to 'config'"
        ),
    )
    @click.option(
        "--secrets",
        multiple=True,
        callback=_coerce_json,
        help="A JSON string containing a dictionary.",
    )
    @click.option(
        "--os",
        type=click.Choice(
            ["macos", "win32", "linux", "auto"], case_sensitive=False
        ),
        help="A choice of 'os', must be declared in config/profile.yml",
    )
    @click.option(
        "--deploy",
        type=click.Choice(
            ["local", "dev", "staging", "prod"], case_sensitive=False
        ),
        help="A choice of 'deploy', must be declared in config/profile.yml",
    )
    def cli(path=None, secrets=None, deploy=None, os=None):
        """Parse configuration and secrets, merging all values into a single
        output JSON file.

        Multiple arguments are accepted for --secrets. Each arguments should
        be a JSON map that will be merged with the final config map. All keys
        in --secrets must already be present in a .yml from the config/schema
        folder.

        The output JSON map will be flattened according to PROFILE arguments,
        which should be pairs in the form of [--key value] following the
        other OPTIONS. If the value is found as a nested key in the config
        object, it will be flattened. Example:

        $ python main.py --os mac --deploy dev

        {

           "key1":  {"macos": "mac-value", "win32": "win-value"}

           "key2":  {"dev": "dev-value", "prod": "prod-value"}

        }

        ...flattens to => {"key1": "mac-value", "key2": "dev-value"}

        """
        # Example above has extra spacing due to behavior of the CLI --help.
        profiles = {}
        if os:
            if os == "auto":
                os_name = platform.system()
                if os_name == "Darwin":
                    profiles["os"] = "macos"
                if os_name == "Windows":
                    profiles["os"] = "win32"
                else:
                    profiles["os"] = "linux"
            else:
                profiles["os"] = os
        if deploy:
            profiles["deploy"] = deploy

        result = main_fn(path, secrets=secrets or {}, profiles=profiles)
        result_json = json.dumps(result)

        # Don't print with newline.
        click.echo(result_json, nl=False)

    return cli