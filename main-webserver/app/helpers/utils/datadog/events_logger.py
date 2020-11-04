from app import *
from app.helpers.utils.general.logs import fractalLog

import logging
from flask import current_app
from datadog import initialize, api

# Logging programmatically will be advantageous over doing logs with filters exclusively in the datadog console
# since programmaticaly we can be more precise and it's easier to learn how to use it (i.e. I don't need to read
# a ton of docs on datadog filters/matches and other whatnot); it's also more flexible


def logEvent(title, text="", tags=None, function=None):
    """ 
    Log event will log an event by making a datadog event and sending it to our
    account w/ the api and app keys as well as doing a fractalLog to keep it in our main body of logs.

    the title, text, and tags are datadog parameters that let us create events that are informative
    function is a string which we may want to pass akin to that of fractalLog to use in the future to know what functions
    this is being logged from
    """

    # TODO (adriano) ideally in the future we'll want to initialize once
    datadog_app_key = current_app.config["DATADOG_APP_KEY"]
    datadog_api_key = current_app.config["DATADOG_API_KEY"]

    options = {
        "api_key": datadog_api_key,
        "app_key": datadog_app_key,
    }

    initialize(**options)

    logs = f"Logging Datadog event with title '{title}' and text '{text}' and " + (
        f"tags '{str(tags)}'" if tags else "no tags"
    )

    api.Event.create(title=title, text=text, tags=tags)
