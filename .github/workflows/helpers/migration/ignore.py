#!/usr/bin/env python
import config
from pathlib import Path

def is_ignored_head(ln: str):
    for head in config.IGNORE_HEAD:
        if ln.startswith(head):
            return True
    return False


def is_ignored_complete(ln: str):
    return ln in config.IGNORE_COMPLETE

def ignore_filter(lines):
    """Filters pre-configured lines from a string of SQL commands

    Certain SQL statements (like ones that generate random values) will always
    appear in the SQL schema diff. This function filters pre-configured SQL
    strings from the diff output.
    """
    no_empty = (ln for ln in lines if ln)
    ignored_head = (ln for ln in no_empty if not is_ignored_head(ln))
    ignored_complete = (ln for ln in ignored_head if not is_ignored_complete(ln))
    return list(ignored_complete)
