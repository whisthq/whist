from app import *
from app.helpers.utils.general.logs import fractalLog

import logging
from time import time # TODO WTF
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
    CLUSTER_NAME as CLUSTER_NAME_F,  # cluster_name=
    SUCCESS,
    FAILURE,
    validTags,
)

# TODO (adriano) ideally in the future we'll want to initialize once
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


def datadogEvent(title, tags, text="", init=True):
    """Logs a datadog event (not the same as a regular log, basically.. a noteworthy log)
    to our account w/ the api and app keys as well as doing a fractalLog to keep it in
    our main body of logs. We require tags to now allow people to make events that are not
    easily categorizeable. We require 2 tags, one for the event-type and one for the status.

    Args:
        title (str): The title of the event giving cursory information.
        text (str, optional): Body text of the event, giving detailed information. Defaults to "".
        tags (List[str]): Tags to tag the event with for filtering. Defaults to None.
    """

    if not validTags(tags):
        fractalLog(
                function="datadogEvent",
                label=None,
                logs=f"Tried to log a datadog event with invalid tags: {str(tags)}",
                level=logging.ERROR,
            )
        raise Exception("Datadog event tags were not valid.")

    if init:
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
    datadogEvent(
        title="Deleted Container",
        text=f"Container {container_name} in cluster {container_cluster}",
        tags=[
            CONTAINER_DELETION,
            SUCCESS,
            CONTAINER_NAME_F.format(container_name=container_name),
            CLUSTER_NAME_F.format(cluster_name=cluster_name),
        ],
    )

    datadogEvent_containerLifecycle(container_name)


def datadogEvent_clusterDelete(cluster_name):
    datadogEvent(
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
        container_name (str): The name of the container whose lifecycle we are observing the end for.
    """
    dt = 100
    d, max_d = 100, 100 * 10

    end_time, start_time = time.time(), end_time - dt
    deletion_date = end_time

    while d < max_d:
        events = api.Event.query(
            start=start_time,
            end=end_time,
            # sources ?
            tags=[CONTAINER_NAME_F.format(container_name=container_name)],
            unaggregated=True
        )

        try:
            # https://docs.datadoghq.com/api/v1/events/
            event = max(events, key=lambda event: event["date_happened"])

            creation_date = event["date_happened"]

            # find everything as a string so we can embedd it in a format string
            runtime = str(deletion_date - creation_date)
            creation_date, deletion_date = str(creation_date), str(deletion_date)

            datadogEvent(
                title="Cluster Lifecycle Ended",
                text=f"Cluster {cluster} has been deleted, completing its lifecycle. " + 
                     f"Creation time was {creation_date} and deletion time was {deletion_date}. " + 
                     f"Change in time was {runtime}.",
                tags=[
                    CONTAINER_LIFECYCLE, 
                    SUCCESS,
                    CONTAINER_NAME_F.format(container_name=container_name)
                ],
            )
        except:
            d += dt
            end_time, start_time = start_time, start_time - dt


    fractalLog(
            function="datadogEvent_containerLifecycle",
            label=None,
            logs=f"Failed to find creation event for cluster {cluster_name}",
            level=logging.ERROR,
        )
    # we will not raise an exception since this may be a common occurence
    # for long-lived containers and/or when we beign to use this functionality
    # unlike datadogEvent where we absolutely want valid tags and the exception will
    # never be invoked except in local testing (where we want to except), here this
    # could be invoked in a real environment and do not want to break the server
