#!/usr/bin/env python3

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
# Functions is this file should be small and contain very little
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
    empty = lambda: ""
    newlined = newline(empty)
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
    return wrap_text + " " * spacing + text + " " * spacing + wrap_text


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


def heading1(text=None):
    if text:
        return f"# {text}"


def heading2(text=None):
    if text:
        return f"## {text}"


def heading3(text=None):
    if text:
        return f"### {text}"


def heading4(text=None):
    if text:
        return f"#### {text}"


def heading5(text=None):
    if text:
        return f"##### {text}"


def sep():
    return "---"


def html_comment(text):
    return f"<!-- {text} -->"


def code_block(text, lang=""):
    if not text:
        return text
    return join_newline("```" + lang, text, "```")


@collapsed
def code_collapsed(*args, **kwargs):
    return code_block(*args, **kwargs)


def code_overflow_collapsed(text, *args, **kwargs):
    if _should_collapse(text):
        return code_collapsed(text, *args, **kwargs)
    return code_block(text, *args, **kwargs)


def identity(i):
    """The identity function

    Somehow, Python doesn't have this built in.

    Args:
       i: anything
    Returns
       i, completely unchanged
    """
    return i


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


def default_message_github(body, title=None, code=None, lang=None):
    """Formats a message to be used as a github comment body
    Args:
        body: a string, the main content of the comment
        title: a optional string, formatted at the top of the comment
        code: a optional string, placed in a block at the bottom of the comment
        lang: a optional string, used to format the comment's code block
    Returns:
        A string containing the parameters, formatted to post as a comment
    """
    return join_newline(heading2(title), body, code_overflow_collapsed(code, lang=lang))


def default_message_slack(body, title=None, code=None, lang=None):
    """Formats a message to be used as a slack post body
    Args:
        body: a string, the main content of the post
        title: a optional string, formatted at the top of the post
        code: a optional string, placed in a block at the bottom of the post
        lang: a optional string, used to format the post's code block
    Returns:
        A string containing the parameters, formatted to post to slack
    """
    # Slack doesn't support most markdown formatting, so we need to keep the
    # message very simple.
    # It also doesn't support the language argument, so we'll just omit it.
    _ = lang
    lines = []
    if title is not None:
        lines.append("*" + title + "*")
    lines.append(body)
    if code is not None:
        lines.append(code_block(code))
    return "\n".join(lines)
