#!/usr/bin/env python
import config
from pathlib import Path


def is_ignored_head(ln: str):
    """Test if a string begins with a IGNORE prefix.

    If a string starts with a substring that is in the set config.IGNORE_HEAD,
    it should be ignored.

    Args:
        ln: A string.
    Returns:
        True if the string starts with a substring in config.IGNORE_HEAD,
        False otherwise.
    """
    for head in config.IGNORE_HEAD:
        if ln.startswith(head):
            return True
    return False


def is_ignored_complete(ln: str):
    """Test if a string is in the set of ignored statements.

    Args:
        ln: A string.
    Returns:
        True if the string is in the set config.IGNORE_COMPLETE,
        False otherwise.
    """
    return ln in config.IGNORE_COMPLETE


def ignore_filter(lines):
    """Filters pre-configured lines from a string of SQL commands

    Certain SQL statements (like ones that generate random values) will always
    appear in the SQL schema diff. This function filters pre-configured SQL
    strings from the diff output.

    Args:
        lines: A list of strings to filter, based on IGNORE configuration.
    Returns:
        A list of strings, with statements to IGNORE filtered out.
    """
    no_empty = (ln for ln in lines if ln)
    ignored_head = (ln for ln in no_empty if not is_ignored_head(ln))
    ignored_complete = (ln for ln in ignored_head if not is_ignored_complete(ln))
    return list(ignored_complete)
