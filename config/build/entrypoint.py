#!/usr/bin/env python3
#
# This file sets up execution with a GitHub Actions environment. It should
# be the ENTRYPOINT in the Dockerfile for the migration Action.
# Here, we set up the postgres server and some empty databases that we'll use
# to compare schemas. After setting up, we call the main diff process to
# actually do the work.
#
# The config/folder must be copied to /root in the container, so that this
# file's path is /root/build/entrypoint.py.
#
# We need to make some GitHub Actions-specific considerations revolving
# around output and exit code, so in some places we check for the CI env
# variable which will be true in the GitHub runner. We want this file to
# be able to run locally for development and testing, so we shouldn't
# make any Action-specific decisions without checking for CI first.

import os
import re
from sys import exit, argv, stderr, stdout
from subprocess import run
from rich import traceback
from collections.abc import Iterable

# Better errors!
traceback.install()


def flatten(l):
    for el in l:
        if isinstance(el, Iterable) and not isinstance(el, (str, bytes)):
            yield from flatten(el)
        else:
            yield el


def actionify(name, data):
    """Given a string name and any type of data, return a string
    formatted for printing to a GitHub runner's a Workflow or Action
    output.
    The name will become the name of the output, and the data will
    be coerced to a string and become the value. Note that for the
    value to be accessible from a Workflow, it must be registered in
    the "outputs" section of an Action or a Workflow job step.
    Any \n, \r, and % characters will be escaped, and automatically
    un-escaped by the GitHub runner.
    """
    escaped = str(data)
    escaped = re.sub("%", "%25", escaped)
    escaped = re.sub("\n", "%0A", escaped)
    escaped = re.sub("\r", "%0D", escaped)
    return f"::set-output name={name}::{escaped}"


# Below, we run /root/build/main.py two different ways under two different
# conditions. If we're in CI, then we look for arguments from environment
# variables. GitHub Actions will pass the arguments through variables
# prefixed with INPUT_. If we're not in CI, then we expect that we are
# running locally, and we accept arguments through command line --flags.

if os.environ.get("CI"):
    maps = {
        "--os": os.environ.get("INPUT_OS"),
        "--deploy": os.environ.get("INPUT_DEPLOY"),
    }

    args = list(flatten(((k, v) for k, v in maps.items() if v is not None)))

    result = run(
        ["python", "/root/build/main.py", "--path", "/root", *args],
        text=True,
        capture_output=True,
        check=False,
    )
    # We're running in CI through a GitHub Action, which means we
    # won't be using the stdout from this process. We need to
    # print the special set-output string to use the result.
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr)
        exit(result.returncode)
    else:
        stdout.write(actionify("config", result.stdout))
else:
    result = run(
        ["python", "/root/build/main.py", "--path", "/root", *argv[1:]],
        text=True,
        capture_output=True,
        check=False,
    )
    # We're not in CI, and we may want to use the stdout directly,
    # so we won't print any headers.
    stdout.write(result.stdout)
    if result.stderr:
        print(result.stderr, file=stderr)
        exit(result.returncode)
