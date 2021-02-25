#!/usr/bin/env python
import os
from functools import wraps
from github import Github


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
    comments = get_comments_by(issue_number, func)
    for comment in comments:
        comment.edit(body)
    if create and not comments:
        create_comment(issue_number, body)
