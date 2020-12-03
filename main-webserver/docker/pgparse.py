#!/usr/bin/env python3

"""pgparse.py

Read a URI from standard input and parses it into its component parts. Print a
series of lines of the form KEY=VALUE to standard output.

This script must be run in a Python 3 interpreter because f-string syntax was
not introduced until Python 3.
"""

from urllib.parse import urlparse


if __name__ == "__main__":
    result = urlparse(input())

    print(f"POSTGRES_DB={result.path[1:]}")
    print(f"POSTGRES_HOST={result.hostname}")
    print(f"POSTGRES_PASSWORD={result.password}")
    print(f"POSTGRES_USER={result.username}")
