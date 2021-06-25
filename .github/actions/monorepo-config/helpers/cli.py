# This file creates the CLI for the configuration program. It's designed
# to have as little knowledge as possible about what goes on in the
# configuration program, which is why its import list is so small.
# This file only knows about the CLI considerations, and that we're expecting
# input and output as JSON.
import json
import click


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
        return dicts
    except json.JSONDecodeError as err:
        raise click.BadParameter(f"{type(err).__name__}: {err.args[0]}")


# GitHub Actions can only pass arguments through environment variables
# to a Docker process. It also insists on prefixing those values with
# INPUT_ and capitalizing them.
#
# For this reason, we configure our CLI tool to also accept environment
# variables in place of the command line flag arguments.
# Note that if both an environment variable and a command line argument are
# passed, the command line argument will override the environment variable.


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

    @click.command()
    @click.argument("path", type=click.Path(exists=True))
    @click.option(
        "-o",
        "--out",
        multiple=True,
        type=click.File("w"),
        envvar="INPUT_OUT",
        help="A target file path to write output JSON.",
    )
    @click.option(
        "-p",
        "--profile",
        multiple=True,
        envvar="INPUT_PROFILE",
        help="A single profile from the list in profiles.yml.",
    )
    @click.option(
        "-s",
        "--secrets",
        multiple=True,
        callback=_coerce_json,
        envvar="INPUT_SECRETS",
        help="A JSON string containing a dictionary.",
    )
    def cli(path, secrets=(), profile=(), out=()):
        """Parse configuration and secrets, merging all values into a single
        output JSON file.

        PATH is the path to the config folder, which should contain a file
        called "profiles.yml", and a folder called "schema".

        The output JSON map should be flat with no nested objects. You can
        control the flattening process by passing --profile arguments. In a
        nested config map, the key that matches a passed profile will
        be the one chosen.

        All CLI flags can also be passed through environment variables by
        capitalizing the name and prefixing with INPUT, e.g. INPUT_PROFILE.
        Pass multiple arguments in a variable by separating with a space.

        ALl CLI flags can be passed multiple times, and the arguments will
        be merged into the final JSON object. Multiple --out files can be
        given, and all will be written to.
        """
        result = main_fn(path, secrets=secrets, profiles=profile)
        result_json = json.dumps(result, indent=4)

        if out:
            for writer in out:
                writer.write(result_json)
                writer.write("\n")
        else:
            click.echo(result_json)

    return cli
