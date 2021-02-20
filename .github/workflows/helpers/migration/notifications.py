import os
import sys
import formatters
from rich import traceback, print
from toolz import partition, identity
from functools import wraps
from github import Github

traceback.install()

GITHUB_REPO = "fractal/fractal"

GITHUB_TOKEN = os.environ.get("GITHUB_TOKEN")

GITHUB_ISSUE_NUMBER = os.environ.get("GITHUB_ISSUE_NUMBER")

GITHUB_BOT_IDENTIFIER = formatters.comment(
    "AUTOMATED MIGRATION SCRIPT MESSAGE")

FORMAT_STDOUT = "FORMAT_STDOUT"

github_connection = Github(GITHUB_TOKEN)


def with_github(func):
    """A decorator to pass a configured GitHub client to functions.

    Passes a default GitHub client as the first argument to decorated
    functions.

    Args:
        func: A function that accepts a GitHub client as its first argument.
    Returns:
        The input function, partially applied with the GitHub client object.
    """
    @wraps(func)
    def with_github_wrapper(*args, **kwargs):
        return func(github_connection, *args, **kwargs)

    return with_github_wrapper


def is_bot_message(string):
    if not isinstance(string, str):
        return False
    return string.split("\n")[0] == GITHUB_BOT_IDENTIFIER


@with_github
def delete_bot_comments(client, issue_number):
    repo = client.get_repo(GITHUB_REPO)
    issue = repo.get_issue(int(issue_number))

    for comm in issue.get_comments():
        if is_bot_message(comm.body):
            comm.delete()


@with_github
def create_comment(client, issue_number, body):
    repo = client.get_repo(GITHUB_REPO)
    issue = repo.get_issue(int(issue_number))
    issue.create_comment(body)


def iter_replace(old, new, xs):
    return [new if i == old else i for i in xs]


def validate_arguments(exit_code):
    if exit_code is None:
        raise Exception(
            "This script wasn't passed an exit code as its"
            + " first argument. There's nothing to do without one, "
            + " so I'm exiting with an error."
        )

    if not GITHUB_TOKEN:
        raise Exception(
            "A GITHUB_TOKEN environment"
            + " variable must be set to continue.")

    if not GITHUB_ISSUE_NUMBER:
        raise Exception(
            "A GITHUB_ISSUE_NUMBER environment"
            + " variable must be set to continue.")


MESSAGES = {
        0: ("valid", "Schema is unchanged, no database migration needed.",
            "body", "Carry on!"),
        1: ("error", "An error occured while comparing the database schema.",
            "body", "The diff tool `migra` exited with an error.",
            "python", FORMAT_STDOUT),
        2: ("notice", "There's some changes to be made to the schema!",
            "body", "The SQL commands below will perform the migration.",
            "sql", FORMAT_STDOUT),
        3: ("alert", ("This PR introduces some destructive changes"
                      + " to the schema!"),
            "body", ("The schema diff produced some unsafe statements. "
                     + "These might include `DROP COLUMN` or `ALTER TABLE` "
                     + "commands, which can be dangerous to run on the "
                     + "database. "
                     + "Remember these will be run automatically upon merge, "
                     + "so be sure to review these changes extra carefully."
                     + "\n\nThe SQL commands below will perform "
                     + " the migration."),
            "sql", FORMAT_STDOUT),
    }

if __name__ == "__main__":

    # Parsing arguments passed to the script
    EXIT_CODE = int(sys.argv[1]) if len(sys.argv) > 1 else None
    DATA = sys.argv[2] if len(sys.argv) > 2 else ""

    # Check that an exit code argument was passed
    validate_arguments(EXIT_CODE)

    # Choose the appropriate message, or default to 1 (general error)
    # Replace the template string (FORMAT_STDOUT) with
    # the scripts second argument
    message_with_stdout = iter_replace(
        FORMAT_STDOUT, DATA, MESSAGES.get(EXIT_CODE, MESSAGES[1]))

    # Spit the chosen message up into "function" "text" pairs
    message_pairs = partition(2, message_with_stdout)

    # Grab the correspoding function from the formatting library
    # If the "function" in the "function", "text" pairs doesn't
    # exist, just leave the text as is (identity function)
    body = formatters.join_newline(
        GITHUB_BOT_IDENTIFIER,
        *(getattr(formatters, func, identity)(message)
          for func, message in message_pairs))

    # Delete the bot's previous messages in the issue.
    # These are identified by checking the first line
    # for the GITHUB_BOT_IDENTIFIER flag, which is formatted
    # as an invisible Markdown comment.
    delete_bot_comments(GITHUB_ISSUE_NUMBER)

    # Write the formatted Markdown body as a new GitHub comment.
    # By deleting and then making a new comment each time, we can make
    # sure that important migration messages aren't buried in long
    # PR conversations.
    create_comment(GITHUB_ISSUE_NUMBER, body)
