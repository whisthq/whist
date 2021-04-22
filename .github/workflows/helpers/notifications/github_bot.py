#!/usr/bin/env python
import os
from functools import wraps, partial
import sys

# add the current directory to the path no matter where this is called from
sys.path.append(os.path.join(os.getcwd(), os.path.dirname(__file__), "."))

from github import Github

import formatters as fmt


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
    create_comment(issue, fmt.default_message_github(body, title, code, lang))


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
    edit_comments_by(
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
    delete_comments_by(issue, partial(filter_identifier, identifier))
    create_comment(issue, fmt.concat_id(identifier, content))


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


def with_github(func):
    """A decorator to pass a configured GitHub repository to functions

    Passes a default GitHub repository to decorated functions as the
    'repo=' kwarg.

    Args:
        func: A function that accepts a GitHub client as its first argument.
    Returns:
        The input function, partially applied with the GitHub client object.
    """

    @wraps(func)
    def with_github_wrapper(*args, **kwargs):

        # GITHUB_REPO to the "fractal/fractal" repository
        # GITHUB_TOKEN must be set, or we'll get an auth error
        github_repo = os.environ.get("GITHUB_REPO", "fractal/fractal")
        github_token = os.environ.get("GITHUB_TOKEN")

        if not github_token:
            raise Exception("No GITHUB_TOKEN environment variable provided.")

        client = Github(github_token)
        repo = client.get_repo(github_repo)

        kwargs["repo"] = kwargs.get("repo", repo)

        return func(*args, **kwargs)

    return with_github_wrapper


@with_github
def create_comment(issue_number, body, repo=None):
    """Create a comment on a GitHub issue or pull request

    Args:
        issue_number: a number representing the issue or PR
        body: a string that becomes the entire content of the comment
        repo: a PyGithub repository object
    Returns:
        None
    """
    issue = repo.get_issue(int(issue_number))
    issue.create_comment(body)


@with_github
def get_comments_by(issue_number, func, repo=None):
    """Retrieve comments from an issue that pass a filter

    FUNC will be passed the PyGithub IssueComment object, and should
    return True or False to filter comments from the returned sequence.

    IssueComment.body represents the text content of the issue, which is
    a common attribute to filter with.

    Args:
        issue_number: a number representing the issue or PR
        func: a function that receives a PyGithub object, return True or False
        repo: a PyGithub repository object
    Returns:
        None
    """
    issue = repo.get_issue(int(issue_number))
    for comment in issue.get_comments():
        if func(comment):
            yield comment


def delete_comments_by(issue_number, func):
    """Delete comments from an issue that pass a filter

    FUNC will be passed the PyGithub IssueComment object, and should
    return True or False to filter comments from the collection to be deleted.

    IssueComment.body represents the text content of the issue, which is
    a common attribute to filter with.

    Args:
        issue_number: a number representing the issue or PR
        func: a function that receives a PyGithub object, return True or False
    Returns:
        None
    """
    for comment in get_comments_by(issue_number, func):
        comment.delete()


def edit_comments_by(issue_number, func, body, create=False):
    """Edits all comments in an issue that pass a filter

    FUNC will be passed the PyGithub IssueComment object, and should
    return True or False to filter comments from the collection to be deleted.

    IssueComment.body represents the text content of the issue, which is
    a common attribute to filter with.

    Args:
        issue_number: a number representing the issue or PR
        func: a function that receives a PyGithub object, return True or False
        body: a string that becomes the entire content of the comment
    Returns:
        None
    """
    comments = list(get_comments_by(issue_number, func))
    for comment in comments:
        comment.edit(body)
    if create and not comments:
        create_comment(issue_number, body)
