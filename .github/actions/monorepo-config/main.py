# This file is the entrypoint into the configuration parsing program.
# It should be run as a main script, and the working directory should be
# the root of the monorepo.
#
# This file is intended to do as little as possible, it just creates the CLI
# and passes the parsing function. This allows the parsing function to be
# imported on its own, and keeps the CLI from having to know about the
# other helper functions.
from rich import traceback
from helpers.parse import parse
from helpers.cli import create_cli

# Rich provides much better error messages.
traceback.install()

if __name__ == "__main__":
    cli = create_cli(parse)
    cli(None)  # Pass an argument to quiet the linter. It's ignored.
