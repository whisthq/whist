import random
import re
import string
from typing import Optional
from flask import current_app


def generate_name(
    starter_name: str = "",
    test_prefix: bool = False,
    branch: Optional[str] = None,
    commit: Optional[str] = None,
):
    """
    Helper function for generating a name with a random UID
    Args:
        starter_name (Optional[str]): starter string for the name
        test_prefix (Optional[bool]): whether the resource should have a "test-" prefix attached
        branch (Optional[str]): Which branch this instance is being created on
        commit (Optional[str]): Which commit this instance is being created on
    Returns:
        str: the generated name
    """
    branch = branch if branch is not None else current_app.config["APP_GIT_BRANCH"]
    commit = commit if commit is not None else current_app.config["APP_GIT_COMMIT"][0:7]
    # Sanitize branch name to prevent complaints from boto. Note from the
    # python3 docs that if the hyphen is at the beginning or end of the
    # contents of `[]` then it should not be escaped.
    branch = re.sub("[^0-9a-zA-Z-]+", "-", branch, count=0, flags=re.ASCII)

    # Generate our own UID instead of using UUIDs since they make the
    # resource names too long (and therefore tests fail). We would need
    # log_36(16^32) = approx 25 digits of lowercase alphanumerics to get
    # the same entropy as 32 hexadecimal digits (i.e. the output of
    # uuid.uuid4()) but 10 digits gives us more than enough entropy while
    # saving us a lot of characters.
    uid = "".join(random.choice(string.ascii_lowercase + string.digits) for _ in range(10))

    # Note that capacity providers and clusters do not allow special
    # characters like angle brackets, which we were using before to
    # separate the branch name and commit hash. To be safe, we don't use
    # those characters in any of our resource names.
    name = f"{starter_name}-{branch}-{commit}-uid-{uid}"

    test_prefix = test_prefix or current_app.testing
    if test_prefix:
        name = f"test-{name}"

    return name
