#!/usr/bin/env python3
import os
import json
import click
import toolz
from sources import heroku
from pathlib import Path

SOURCES = [heroku]


def yield_inputs(env):
    for key, value in env.items():
        if key.startswith("INPUT_"):
            new_key = key[len("INPUT_") :].lower()
            if new_key == "secrets":
                yield new_key, json.loads(value)
            else:
                yield new_key, value


def parse_inputs(env):
    if not env.get("INPUT_SECRETS"):
        raise Exception("Environment INPUT_SECRETS= must be a JSON string.")
    return dict(yield_inputs(env))


def main():
    inputs = parse_inputs(os.environ)
    return toolz.merge(inputs["secrets"], *(src(**inputs) for src in SOURCES))


if __name__ == "__main__":
    print(main())
