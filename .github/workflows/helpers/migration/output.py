#!/usr/bin/env python
from functools import wraps


def title(text):
    """Print a formatted Markdown message title.

    Args:
        text: A string that is the body of the title.

    Returns:
        None
    """
    print(f"## :construction: {text} :construction:")


def alert(text):
    """Print a formatted Markdown message alert.

    Args:
        text: A string that is the body of the alert.

    Returns:
        None
    """
    print(f"## :rotating_light: {text} :rotating_light:")


def body(text):
    """Print a formatted Markdown message.

    Args:
        text: A string that is the message to be printer.

    Returns:
        None
    """
    print(text)


def sep():
    print("---")


def collapse(func):
    """A decorator to wrap "collapsible" Markdown around output.

    Args:
        text: A function that prints output that should collapse.

    Returns:
        A function that prints "collapsible" Markdown around the output
        of the wrapped function.
    """

    @wraps(func)
    def collapse_wrapper(*args, **kwargs):
        print("<details>")
        print("<summary>Click to expand</summary>\n")
        func(*args, **kwargs)
        print("</details>")

    return collapse_wrapper


@collapse
def sql(text):
    """Print a formatted Markdown string of sql statemens

    Args:
        text: A string of SQL statements.

    Returns:
        None
    """
    print("```sql")
    print(text)
    print("```")
