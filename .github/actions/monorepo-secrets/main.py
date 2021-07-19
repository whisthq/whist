#!/usr/bin/env python
#!/usr/bin/env python3
import json
import click
import toolz
from sources import heroku
from pathlib import Path

SOURCES = [heroku]


def take_options(pairs):
    for pair in toolz.partition(2, pairs):
        if not (isinstance(pair[0], str) and pair[0].startswith("--")):
            break
        yield (pair[0][2:], pair[1])


def parse_json(j):
    if not isinstance(j, str):
        raise click.UsageError(
            "Arguments after --options must be JSON strings."
        )
    try:
        result = json.loads(j)
    except json.decoder.JSONDecodeError:
        result = j
    if not isinstance(result, dict):
        raise click.UsageError(
            "Arguments after --options must be JSON that decode to dict."
        )
    return result


def parse_option_pairs(pairs):
    options = dict(take_options(pairs))
    rest = pairs[len(options) * 2 :]
    return toolz.merge(*(parse_json(r) for r in rest), options)


def parse_arguments(_ctx, _param, args):
    return parse_option_pairs(args)


def merge_sources(secrets):
    return toolz.merge(secrets, *(src(secrets) for src in SOURCES))


def create_cli(main_fn):
    @click.command(context_settings=dict(ignore_unknown_options=True))
    @click.argument("secrets", nargs=-1, callback=parse_arguments)
    def cli(secrets):
        """Parse secrets, merging all values into a single
        output JSON file.
        """
        print(json.dumps(main_fn(secrets)))

    return cli(None)  # Pass an argument to quiet linter.


if __name__ == "__main__":
    create_cli(merge_sources)
