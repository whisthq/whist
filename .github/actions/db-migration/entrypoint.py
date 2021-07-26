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
from sys import exit, argv
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
    # We also need to do some escaping of single-quotes because the output of
    # this script is sent to the deployment script, via GHA, wrapped in single
    # quotes.
    escaped = re.sub("'", "'\\''", escaped)
    return f"::set-output name={name}::{escaped}"


# We're installing postgres-13 in our docker file. The version in the
# command below needs to match.

# These commands start a postgres server, creates two databases, and then
# runs the arguments to this script as a process.

run("pg_ctlcluster 13 main start".split(), check=True)
run(["psql", "--quiet", "--command", "create database a;"], check=True)
run(["psql", "--quiet", "--command", "create database b;"], check=True)
result = run(argv[1:], text=True, capture_output=True, check=False)

code = result.returncode
diff = result.stdout

if os.environ.get("CI"):
    # We're running in CI through a GitHub Action, which means we
    # won't be using the stdout from this process. We need to
    # print the special set-output string to use the result.
    # This can also be done in a post-entrypoint.sh if these
    # headers ever get in the way, but we keep it here now for
    # simplicity.
    print("MIGRA CODE:", code)
    print("MIGRA DIFF:\n", diff)
    print(actionify("code", code))
    print(actionify("diff", diff))
else:
    # If we're not in CI, we may want to use the stdout directy,
    # so we won't print any headers.
    print(diff)

# GitHub Actions requires a 0 exit code, or it will crash.
# We accept 2, 3, and 4 exit codes from our diff program, as they
# denominate diff states. We must check for those, and exit 0
# if we find any.

if code == 2:
    exit(0)
if code == 3:
    exit(0)
if code == 4:
    exit(0)

# We haven't exited yet, which means we got a return code we did
# not expect (e.g. Migra exits with 1 if there was an error).
# For any unexpected return code, we'll print stdout/stderr so
# we can see what's going on,
print(result.stderr)
print(result.stdout)
exit(result.returncode)
