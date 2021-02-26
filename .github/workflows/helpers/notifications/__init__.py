#!/usr/bin/env python
from functools import wraps, partial
from . import formatters as fmt
from . import github_bot
from . import slack_bot


def apply_format(func, **kwargs):
    """Apply a function to notification keyword arguments

    A internal function used to apply formatting to the keyword arguments
    that configure notification functions.

    Args:
        func: a function to apply to keyword arguments
        kwargs: a dictionary of keyword arguments
    Returns:
        a dictionary of keyword arguments"""
    kwargs["title"] = func(kwargs["title"])
    return kwargs


def filter_identifier(identifier, comment):
    """Check if a comment body starts with a unique identifier

    Args:
        identifier: a string, representing a unique id for a comment
        comment: a PyGithub object with a 'body' attribute
    Returns:
        True or False
    """
    return fmt.startswith_id(identifier, comment.body)


# These small functions are partial applications of "apply_format"
# They "configure" it for use with each logging level
# Title formatting and default values can be changed here
format_debug = partial(apply_format, fmt.construction, title="DEBUG")
format_info = partial(apply_format, fmt.identity, title="INFO")
format_warning = partial(apply_format, fmt.exclamation, title="WARNING")
format_error = partial(apply_format, fmt.x, title="ERROR")
format_critical = partial(apply_format, fmt.rotating_light, title="CRITICAL")


def github_comment(issue, body, title=None, code=None, lang=None):
    """Creates a comment on a GitHub issue or PR

    Requires the environment variable GITHUB_TOKEN to be set.

    Defaults to the fractal/fractal repository. The environment variable
    GITHUB_REPO can be set to choose a different repository.

    Args:
        issue: a number that represents the issue or pull request
        body: a string, the main content of the comment
        title: a optional string, formatted at the top of the comment
        code: a optional string, placed in a block at the bottom of the comment
        lang: a optional string, used to format the comment's code block
    Returns
        None"""
    github_bot.create_comment(issue,
                              fmt.default_message_github(body,
                                                         title,
                                                         code,
                                                         lang))


def github_comment_update(issue, identifier, body, title=None, code=None, lang=None):
    """Updates a previous auto-comment on a GitHub issue or PR

    A IDENTIFIER must be passed as the second argument, which will be hidden
    in the markup of the comment to identify it to later runs of your script.

    If this function finds one or more comments in the thread containing
    IDENTIFIER, it will replace the contents of that comment with BODY.


    Args:
        issue: a number that represents the issue or pull request
        identifier: a string that represents 'instances' of this comment
        body: a string, the main content of the comment
        title: a optional string, formatted at the top of the comment
        code: a optional string, placed in a block at the bottom of the comment
        lang: a optional string, used to format the comment's code block
    Returns:
        None"""
    content = fmt.default_message_github(body, title, code, lang)
    github_bot.edit_comments_by(
        issue,
        partial(filter_identifier, identifier),
        fmt.concat_id(identifier, content),
        create=True,
    )


def github_comment_once(issue, identifier, body, title=None, code=None, lang=None):
    """Cleans up old auto-comments and creates a new one on a GitHub issue or PR

    A IDENTIFIER must be passed as the second argument, which will be hidden
    in the markup of the comment to identify it to later runs of your script.

    If this function finds one or more comments in the thread containing
    IDENTIFIER, it will delete those comments. A new comment is always posted
    at the bottom of the thread

    Requires the environment variable GITHUB_TOKEN to be set.

    Defaults to the fractal/fractal repository. The environment variable
    GITHUB_REPO can be set to choose a different repository.

    Args:
        issue: a number that represents the issue or pull request
        identifier: a string that represents 'instances' of this comment
        body: a string, the main content of the comment
        title: a optional string, formatted at the top of the comment
        code: a optional string, placed in a block at the bottom of the comment
        lang: a optional string, used to format the comment's code block
    Returns:
        None"""
    content = fmt.default_message_github(body, title, code, lang)
    github_bot.delete_comments_by(issue,
                                  partial(filter_identifier, identifier))
    github_bot.create_comment(issue, fmt.concat_id(identifier, content))


def slack_post(channel, body, title=None, code=None, lang=None):
    """Creates a post on a Slack channel

    Requires the environment variable SLACK_TOKEN to be set.

    Defaults to using the Fractal Bot as username. The environment variable
    SLACK_USERNAME can be set to choose a different username.

    Args:
        channel: a string that represents the channel to post to
        body: a string, the main content of the comment
        title: a optional string, formatted at the top of the comment
        code: a optional string, placed in a block at the bottom of the comment
        lang: a optional string, used to format the comment's code block
    Returns
        None"""
    slack_bot.create_post(channel,
                          fmt.default_message_slack(body, title, code, lang),
                          text=(title or body))


# The functions below represent the main logging API for each service.
# They have the same behavior and function signatures as the functions
# above, which they wrap. In the spirit of brevity, we're decorating each one
# with functools.wraps, passing in ["__doc__"] as the second argument.
# This leaves each function's __name__ intact, but gives it the signature
# and documentation string of one of the functions above.
#
# Each function will have the correct __doc__ attribute, and will have the
# expected behavior when you call help(...) on any one of them.


@wraps(github_comment, ["__doc__"])
def github_debug(*args, **kwargs):
    return github_comment(*args, **format_debug(**kwargs))


@wraps(github_comment, ["__doc__"])
def github_info(*args, **kwargs):
    return github_comment(*args, **format_info(**kwargs))


@wraps(github_comment, ["__doc__"])
def github_warning(*args, **kwargs):
    return github_comment(*args, **format_warning(**kwargs))


@wraps(github_comment, ["__doc__"])
def github_error(*args, **kwargs):
    return github_comment(*args, **format_error(**kwargs))


@wraps(github_comment, ["__doc__"])
def github_critical(*args, **kwargs):
    return github_comment(*args, **format_critical(**kwargs))


@wraps(github_comment_once, ["__doc__"])
def github_debug_once(*args, **kwargs):
    return github_comment_once(*args, **format_debug(**kwargs))


@wraps(github_comment_once, ["__doc__"])
def github_info_once(*args, **kwargs):
    return github_comment_once(*args, **format_info(**kwargs))


@wraps(github_comment_once, ["__doc__"])
def github_warning_once(*args, **kwargs):
    return github_comment_once(*args, **format_warning(**kwargs))


@wraps(github_comment_once, ["__doc__"])
def github_error_once(*args, **kwargs):
    return github_comment_once(*args, **format_error(**kwargs))


@wraps(github_comment_once, ["__doc__"])
def github_critical_once(*args, **kwargs):
    return github_comment_once(*args, **format_critical(**kwargs))


@wraps(github_comment_update, ["__doc__"])
def github_debug_update(*args, **kwargs):
    return github_comment_update(*args, **format_debug(**kwargs))


@wraps(github_comment_update, ["__doc__"])
def github_info_update(*args, **kwargs):
    return github_comment_update(*args, **format_info(**kwargs))


@wraps(github_comment_update, ["__doc__"])
def github_warning_update(*args, **kwargs):
    return github_comment_update(*args, **format_warning(**kwargs))


@wraps(github_comment_update, ["__doc__"])
def github_error_update(*args, **kwargs):
    return github_comment_update(*args, **format_error(**kwargs))


@wraps(github_comment_update, ["__doc__"])
def github_critical_update(*args, **kwargs):
    return github_comment_update(*args, **format_critical(**kwargs))


@wraps(slack_post, ["__doc__"])
def slack_debug(*args, **kwargs):
    return slack_post(*args, **format_debug(**kwargs))


@wraps(slack_post, ["__doc__"])
def slack_info(*args, **kwargs):
    return slack_post(*args, **format_info(**kwargs))


@wraps(slack_post, ["__doc__"])
def slack_warning(*args, **kwargs):
    return slack_post(*args, **format_warning(**kwargs))


@wraps(slack_post, ["__doc__"])
def slack_error(*args, **kwargs):
    return slack_post(*args, **format_error(**kwargs))


@wraps(slack_post, ["__doc__"])
def slack_critical(*args, **kwargs):
    return slack_post(*args, **format_critical(**kwargs))
