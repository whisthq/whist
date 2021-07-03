#!/usr/bin/env python
import subprocess


def dump_schema(url):
    result = subprocess.run(
        ["pg_dump", "--no-owner", "--no-privileges", "--schema-only", url],
        capture_output=True,
        check=True,
        text=True,
    )
    return result.stdout


def exec_sql(url, command):
    # psql can accept streamed input through stdin with its --file option.
    # You need to pass a '-' to --file to enable it.

    # We capture the output by default, otherwise psql prints all the sql
    # commands, which clogs the console. If we don't get the return code we
    # expect, we'll manually raise the error and print the psql output.
    cmd = ["psql", "--single-transaction", "--file", "-", url]
    result = subprocess.run(
        cmd,
        capture_output=True,
        input=command,
        check=False,
        text=True,
    )
    if result.returncode != 0:
        print(result.stderr)
        print(result.stdout)
        raise subprocess.CalledProcessError(result.returncode, cmd)
    return result.stdout


def diff_schema(url_A, url_B):
    # Migra return code 1 signifies a migra error. We need to print the
    # migra output manually if we want to see a message traceback. If you
    # don't print, you'll just see a CalledProcessError for this function,
    # and you won't see migra's message.
    try:
        subprocess.run(
            ["migra", "--unsafe", url_A, url_B],
            capture_output=True,
            check=True,
            text=True,
        )

        return 0, None
    except subprocess.CalledProcessError as e:
        if e.returncode == 2:
            return 2, e.stdout
        if e.returncode == 3:
            return 3, e.stdout
        if e.stdout:
            print("stdout", e.stdout)
        print("MIGRA ERROR:\n", e.stderr)
        raise e


def replace_postgres(url):
    """SQLAlchemy, which is a dependency of migra, has changed what its allowed
    URL scheme in recent versions. In case we retrieve an outdated schema"""
    sep = url.index("://")
    return "postgresql" + url[sep:]
