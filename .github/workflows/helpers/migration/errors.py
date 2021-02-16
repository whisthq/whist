#!/usr/bin/env python
import subprocess
import requests
import config
import traceback
from functools import wraps


def format_error(heading, input_data, error_data, suggestion=None):
    """A utility function to build common error messages"""
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
    """A decorator used to catch ValueError and format the message"""

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
    """A decorator used to catch TimeoutError and format the message"""

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

    This function catches the specifc HTTPError emitted by the Requests
    library. While it works for any Requests call, the message is formatted
    to help point the user towards their Heroku credentials.
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
    """A decorator used to catch CalledProcessError and format the message"""

    @wraps(func)
    def catch_process_error_wrapper(*args, **kwargs):
        try:
            return func(*args, **kwargs)

        except subprocess.CalledProcessError as e:
            stderr = e.stderr.decode("utf-8").strip()
            raise format_error(
                f"The following command returned status {e.returncode}:",
                str(e.cmd).strip(),
                stderr,
            ) from None

    return catch_process_error_wrapper
