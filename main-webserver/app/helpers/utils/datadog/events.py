from app import *
from app.helpers.utils.general.logs import fractalLog

import logging
from flask import current_app
from datadog import initialize, api

# ones with F at the end must be formatted
from app.helpers.utils.datadog.event_tags import (
    CONTAINER_CREATION,
    CONTAINER_DELETION,
    CONTAINER_LIFECYCLE,
    CONTAINER_NAME as CONTAINER_NAME_F,  # container_name=
    CLUSTER_CREATION,
    CLUSTER_DELETION,
    CLUSTER_LIFECYCLE,
    CLUSTER_NAME as CLUSTER_NAME_F,  # cluster_name=
    SUCCESS,
    FAILURE,
)


def initDatadog():
    """A simple function that will initialize datadog for our usage in the future.
    This is meant to be called once in a webserver session. We do not want to be re-initializing for
    each http call.
    """
    datadog_app_key = current_app.config["DATADOG_APP_KEY"]
    datadog_api_key = current_app.config["DATADOG_API_KEY"]

    options = {
        "api_key": datadog_api_key,
        "app_key": datadog_app_key,
    }

    initialize(**options)


# Logging programmatically will be advantageous over doing logs with filters exclusively in the datadog console
# since programmaticaly we can be more precise and it's easier to learn how to use it (i.e. I don't need to read
# a ton of docs on datadog filters/matches and other whatnot); it's also more flexible


def datadogEvent(title, text="", tags=None):
    """Logs a datadog event (not the same as a regular log, basically.. a noteworthy log)
    to our account w/ the api and app keys as well as doing a fractalLog to keep it in
    our main body of logs.

    Args:
        title ([str]): The title of the event giving cursory information.
        text (str, optional): Body text of the event, giving detailed information. Defaults to "".
        tags ([List[str]], optional): Tags to tag the event with for filtering. Defaults to None.
    """

    # TODO (adriano) ideally in the future we'll want to initialize once
    initDatadog()

    api.Event.create(title=title, text=text, tags=tags)


"""Below we have some commonly used events. We expect all events to have the following tags (and
potentially more): one tag for the type of event and one tag for success/failure or some other status.
Container creation and deletion events also have tags for their names so that they can be queried by
lifecycle events.
"""


def datadogEvent_containerCreate(container_name, cluster_name):
    datadogEvent(
        title="Created new Container",
        text=f"Container {container.container_id} in cluster {cluster_name}",
        tags=[
            CONTAINER_CREATION,
            SUCCESS,
            CONTAINER_NAME_F.format(container_name=container_name),
            CLUSTER_NAME_F.format(cluster_name=cluster_name),
        ],
    )


def datadogEvent_clusterCreate(cluster_name):
    datadogEvent(
        title="Created new Cluster",
        text=f"Cluster {cluster_name}",
        tags=[CLUSTER_CREATION, SUCCESS, CLUSTER_NAME_F.format(cluster_name=cluster_name)],
    )


def datadogEvent_containerDelete(container_name, cluster_name):
    datadogLog(
        title="Deleted Container",
        text=f"Container {container_name} in cluster {container_cluster}",
        tags=[
            CONTAINER_DELETION,
            SUCCESS,
            CONTAINER_NAME_F.format(container_name=container_name),
            CLUSTER_NAME_F.format(cluster_name=cluster_name),
        ],
    )


def datadogEvent_clusterDelete(cluster_name):
    datadogLog(
        title="Deleted Cluster",
        text=f"Cluster {cluster}",
        tags=[CLUSTER_DELETION, SUCCESS, CLUSTER_NAME_F.format(cluster_name=cluster_name)],
    )


def datadogEvent_containerLifecycle(container_name):
    """The goal is to tell the amount of time a container was up/down.

    This will log an event for the lifecycle of a container that is being shut down.
    It will search for the (most recent )previous event with tags corresponding to
    the creation of the container which is being deleted and then find the distance in time
    and put it into the body along with this event. This is meant to be filtered and then used
    for log analysis elsewhere.

    Note that as a @precondition, the container this is being invoked for must have been created
    while these datadog logs were in effect, AND it must be deleted now. In the case where it is
    not being deleted then this will return false information (i.e. the container lived for more
    than this will log) without an error. If the container's creation was not found, then this
    will fractalLog an error with a corresponding message.

    Args:
        container_name ([str]): The name of the container whose lifecycle we are observing the end for.
    """
    pass
