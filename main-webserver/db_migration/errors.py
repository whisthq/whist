#!/usr/bin/env python
import subprocess
import requests
import config
import traceback
from functools import wraps


def format_error(heading, input_data, error_data, suggestion=None):
    """A utility function to build common error messages.
    Args:
        heading: A string that summarizes the error.
        input_data: A string that provides context about the error, such as
                    a list of arguments passed to a function.
        error_data: A string that provides detail about the error that was
                    raised. The traceback is a useful piece of data.
        suggestion: A optional string that provides a helpful hint on how the
                    user might remedy the error.

    Returns:
        An Exception object constructured with the formatted error message.
        It can be raised directly, for example:

        raise format_error(
                  "A problem happened with the connection on this database:",
                   db.name,
                   error.traceback,
                   "You probably forgot to pass in your access token.")
    """
    suggestion = ("\n\n" + suggestion) if suggestion else ""
    return Exception(
        heading
        + "\n\n"
        + input_data
        + "\n\n...due to the following error:\n\n"
        + error_data
        + suggestion
    )


def catch_value_error(func):
    """A decorator used to catch ValueErrors caused by incorrect arguments.

    Args:
        func: A function that may raise a ValueError.
    Returns:
        A function that will catch any ValueError raised, and raise a
        new error containing useful context, including a list of arguments
        that were passed to the wrapped function.
    Raises:
        Exception: The wrapped function raised a ValueError, which was caught
                   and a formatted Exception was re-raised.
    """

    @wraps(func)
    def catch_value_error_wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except ValueError as e:
            raise format_error(
                "The following function received incorrect arguments:",
                func.__name__,
                (e.args[0] + " Received:"),
                str(list(e.args[1:])),
            ) from e

    return catch_value_error_wrapper


def catch_timeout_error(func):
    """A decorator used to catch TimeoutError and format the message.
    Args:
        func: A function that may raise a TimeoutError.
    Returns:
        A function that will catch any TimeoutError raised, and raise a
        new error containing useful context, including the timeout length.
    Raises:
        Exception: The wrapped function raised a TimeoutError, which was
                   caught and a formatted Exception was re-raised.
    """

    @wraps(func)
    def catch_timeout_error_wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)
        except TimeoutError as e:
            raise format_error(
                (
                    "The following function timed out after "
                    + str(config.TIMEOUT_SECONDS)
                    + " seconds:"
                ),
                func.__name__,
                traceback.format_exc(limit=2),
            ) from e

    return catch_timeout_error_wrapper


def catch_auth_error(func):
    """A decorator used to catch HTTP auth errors and format the message

    This function catches the specific HTTPError emitted by the Requests
    library. While it works for any Requests call, the message is formatted
    to help point the user towards their Heroku credentials.

    Args:
        func: A function that may raise a HTTPError.
    Returns:
        A function that will catch any HTTPError raised, and potentially
        raise a new, formatted error based on the status of the HTTP response.
    Raises:
        Exception: The wrapped function raised a HTTPError with a status code
                   suggesting that there was an authorization problem with the
                   request. A new Exception with helpful formatting was raised.
        HTTPError: The wrapped function raised a HTTPError that was not caused
                   by an authorization problem with the request, so the
                   HTTPError is re-raised.
    """

    @wraps(func)
    def catch_auth_error_wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)

        except requests.HTTPError as e:
            request = e.request
            response = e.response
            status = response.status_code

            if status == 403 or status == 401:
                json = response.json()
                r_id = json["id"]
                r_message = json["message"]
                raise format_error(
                    f"The following HTTP request returned status {status}:",
                    request.url,
                    f"Error: {r_id}. {r_message}",
                    "Is .netrc configured with your Heroku credentials?",
                ) from None

            raise e

    return catch_auth_error_wrapper


def catch_process_error(func):
    """A decorator used to catch CalledProcessError and format the message
    Args:
        func: A function that may raise a subprocess.CalledProcessError.
    Returns:
        A function that will catch any subprocess.CalledProcessError raised,
        and raise a new, formatted Exception.
    Raises:
        Exception: The wrapped function raised a CalledProcessError, which
                   was caught and an Exception with helpful formatting was
                   raised.
    """

    @wraps(func)
    def catch_process_error_wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)

        except subprocess.CalledProcessError as e:
            if isinstance(e.stderr, str):
                stderr = e.stderr
            else:
                stderr = e.stderr.decode("utf-8").strip()
            raise format_error(
                f"The following command returned status {e.returncode}:",
                str(e.cmd).strip(),
                stderr,
            ) from None

    return catch_process_error_wrapper
