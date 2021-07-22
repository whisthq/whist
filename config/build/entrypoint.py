#!/usr/bin/env python3
#
# This file sets up execution with a GitHub Actions environment. It should
# be the ENTRYPOINT in the Dockerfile for the migration Action.
# Here, we set up the postgres server and some empty databases that we'll use
# to compare schemas. After setting up, we call the main diff process to
# actually do the work.
#
# We need to make some GitHub Actions-specific considerations revolving
# around output and exit code, so in some places we check for the CI env
# variable which will be true in the GitHub runner. We want this file to
# be able to run locally for development and testing, so we shouldn't
# make any Action-specific decisions without checking for CI first.

import os
import re
from sys import exit, argv, stderr
from subprocess import run
from rich import traceback

# Better errors!
traceback.install()


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


# The fractal/config folder will get copied to /root inside the container.
# We'll run the build/main.py and pass it the /root as the --path.
# We pull any extra CLI arguments from sys.argv, and pass it to the subprocess.
result = run(
    ["python", "/root/build/main.py", "--path", "/root", *argv[1:]],
    text=True,
    capture_output=True,
    check=False,
)

config = result.stdout

if os.environ.get("CI"):
    # We're running in CI through a GitHub Action, which means we
    # won't be using the stdout from this process. We need to
    # print the special set-output string to use the result.
    print(actionify("config", config))
else:
    # We're not in CI, and we may want to use the stdout directly,
    # so we won't print any headers.
    print(config)

if result.stderr:
    print(result.stderr, file=stderr)
exit(result.returncode)
