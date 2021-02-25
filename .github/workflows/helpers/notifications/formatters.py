#!/usr/bin/env python
from functools import wraps

MAX_COLLAPSE_LINES = 5

# Markdown formatting functions
#
# These functions are simple utilities to create
# Markdown strings that are compatible with all the
# notification services we use, like GitHub and Slack.
#
# At the bottom, we have some higher-level functions
# like "default_message()", which combine formatting
# utilities to produce full title/body/code formatted message.
#
# Functions is this file should be small and container very little
# logic. Their purpose is to accept data and return Markdown
# strings. They do not alter input data or perform any kind
# of validation.


def _should_collapse(text):
    if not text:
        return False
    return len(text.split("\n")) > MAX_COLLAPSE_LINES


def newline(func):
    @wraps(func)
    def newline_wrapper(*args, **kwargs):
        return func(*args, **kwargs) + "\n"

    return newline_wrapper


def join_newline(*args):
    identity = lambda: ""
    newlined = newline(identity)
    return newlined().join(i for i in args if i)


def collapsed(func):
    @wraps(func)
    def collapse_wrapper(*args, **kwargs):
        return join_newline(
            "<details>",
            "<summary>Click to expand</summary>\n",
            func(*args, **kwargs),
            "</details>",
        )

    return collapse_wrapper


def surround_base(spacing, wrap_text, text):
    return wrap_text + (" " * spacing) + text + (" " * spacing) + wrap_text


def surround_xs(wrap_text, text):
    return surround_base(1, wrap_text, text)


def surround_sm(wrap_text, text):
    return surround_base(2, wrap_text, text)


def surround_md(wrap_text, text):
    return surround_base(3, wrap_text, text)


def surround_lg(wrap_text, text):
    return surround_base(4, wrap_text, text)


def surround_xl(wrap_text, text):
    return surround_base(5, wrap_text, text)


def h1(text=None):
    if text:
        return f"# {text}"


def h2(text=None):
    if text:
        return f"## {text}"


def h3(text=None):
    if text:
        return f"### {text}"


def h4(text=None):
    if text:
        return f"#### {text}"


def h5(text=None):
    if text:
        return f"##### {text}"


def sep():
    return "---"


def x(text):
    return surround_sm(":x:", text)


def html_comment(text):
    return f"<!-- {text} -->"


def thumbsup(text):
    return surround_sm(":thumbsup:", text)


def mag_right(text):
    return surround_sm(":mag_right:", text)


def construction(text):
    return surround_sm(":construction:", text)


def rotating_light(text):
    return surround_sm(":rotating_light:", text)


def exclamation(text):
    return surround_sm(":exclamation:", text)


def code(text, lang=""):
    if not text:
        return text
    return join_newline("```" + lang, text, "```")


@collapsed
def code_collapsed(*args, **kwargs):
    return code(*args, **kwargs)


def code_overflow_collapsed(text, *args, **kwargs):
    if _should_collapse(text):
        return code_collapsed(text, *args, **kwargs)
    return code(text, *args, **kwargs)


def identity(x):
    """The identity function

    Somehow, Python doesn't have this built in.

    Args:
       x: anything
    Returns
       x, completely unchanged
    """
    return x


def concat_id(identifier, text):
    """Concatenate a unique identifer with a body of text

    The concatenated identifier will be placed in the first line.
    """
    return join_newline(html_comment(identifier), text)


def startswith_id(identifier, text):
    """Check if a message body's first line is an identifier

    Args:
        identifier: a string used to identify this type of comment
        text: a string to check for an occurence of IDENTIFIER
    Returns:
        True or False
    """
    if not isinstance(text, str):
        return False
    return text.split("\n")[0] == html_comment(identifier)


def default_message(body, title=None, code=None, lang=None):
    """Formats a message to be used as a comment body
    Args:
        body: a string, the main content of the comment
        title: a optional string, formatted at the top of the comment
        code: a optional string, placed in a block at the bottom of the comment
        lang: a optional string, used to format the comment's code block
    Returns:
        A string containing the parameters, formatted to post as a comment
    """
    return join_newline(h2(title), body, code_overflow_collapsed(code, lang=lang))
