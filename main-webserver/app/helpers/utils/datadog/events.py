from app import *
from app.helpers.utils.general.logs import fractalLog

import logging
from time import time  # TODO WTF
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
            logs="Tried to log a datadog event with invalid tags: {tags}".format(tags=str(tags)),
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
    """Logs an event for container creation. This is necessary for lifecycle events to be
    able to work so that they can find the time that the deleted container was created.

    Args:
        container_name (str): The name of the container which was created.
            This is used to search in lifecycle.
        cluster_name (str): The cluster it was created in.
    """
    datadogEvent(
        title="Created new Container",
        text="Container {container_name} in cluster {cluster_name}".format(
            container_name=container_name, cluster_name=cluster_name
        ),
        tags=[
            CONTAINER_CREATION,
            SUCCESS,
            CONTAINER_NAME_F.format(container_name=container_name),
            CLUSTER_NAME_F.format(cluster_name=cluster_name),
        ],
    )


def datadogEvent_clusterCreate(cluster_name):
    """Same as datadogEvent_containerCreate but for clusters.

    Args:
        cluster_name (str): The name of the cluster which was created.
            This is used to search in lifecycle.
    """
    datadogEvent(
        title="Created new Cluster",
        text="Cluster {cluster_name}".format(cluster_name=cluster_name),
        tags=[CLUSTER_CREATION, SUCCESS, CLUSTER_NAME_F.format(cluster_name=cluster_name)],
    )


def datadogEvent_containerDelete(container_name, cluster_name, lifecycle=False):
    """Logs an event for the deletion of the container. It has a "naive" option and a
    lifecycle option. In the lifecycle option it will log a lifecycle type event instead
    of a deletion type event. This event will inform users of the length of time taken.
    In the case of the naive method (lifecycle=False) it will simply log that deletion
    without any calculation etc.

    Args:
        container_name (str): The name of the container that was deleted.
        cluster_name (str): The name of the cluster that the deleted container was in.
        lifecycle (bool, optional): Whether we want to measure time and return a lifecycle type
            event or prefer just to log the deletion itself naively. Defaults to False.
    """
    if lifecycle:
        datadogEvent_containerLifecycle(container_name)
    else:
        datadogEvent(
            title="Deleted Container",
            text="Container {container_name} in cluster {container_cluster}".format(
                container_name=container_name, cluster_name=cluster_name
            ),
            tags=[
                CONTAINER_DELETION,
                SUCCESS,
                CONTAINER_NAME_F.format(container_name=container_name),
                CLUSTER_NAME_F.format(cluster_name=cluster_name),
            ],
        )


def datadogEvent_clusterDelete(cluster_name, lifecycle=False):
    """Same idea as datadogEvent_containerDelete but for clusters.

    Args:
        cluster_name (str): Name of the cluster deleted.
        lifecycle (bool, optional): Whether to do lifecycle type instead of naive. Defaults to False.
    """
    if lifecycle:
        datadogEvent_clusterLifecycle(cluster_name)
    else:
        datadogEvent(
            title="Deleted Cluster",
            text="Cluster {cluster_name}".format(cluster_name=cluster_name),
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
    deletion_date = time.time()

    event = _most_recent_by_tags(
        [CONTAINER_NAME_F.format(container_name=container_name)],
        # TODO in the future maybe use check_event
        # right now we want to see if the first event is a deletion since that it's better not
        # to log a wrong event
    )
    if event:
        if not CONTAINER_CREATION in event["tags"]:
            was_deletion = str(
                CONTAINER_DELETION in event["tags"] or CONTAINER_LIFECYCLE in event["tags"]
            )
            fractalLog(
                function="datadogEvent_containerLifecycle",
                label=None,
                logs="Last event was not a creation for container {container_name}. Was it deletion? Answer: {was_deletion}.".format(
                    container_name=container_name, was_deletion=was_deletion
                ),
                level=logging.ERROR,
            )
        else:
            creation_date = event["date_happened"]

            # find everything as a string so we can embedd it in a format string
            runtime = str(deletion_date - creation_date)
            creation_date, deletion_date = str(creation_date), str(deletion_date)

            datadogEvent(
                title="Container Lifecycle Ended",
                text="Container {container_name} has been deleted, completing its lifecycle. Creation time was {creation_date} and deletion time was {deletion_date}. Change in time was {runtime}.".format(
                    container_name=container_name,
                    creation_date=creation_date,
                    deletion_date=deletion_date,
                    runtime=runtime,
                ),
                tags=[
                    CONTAINER_LIFECYCLE,
                    SUCCESS,
                    CONTAINER_NAME_F.format(container_name=container_name),
                ],
            )
    else:
        fractalLog(
            function="datadogEvent_containerLifecycle",
            label=None,
            logs="Failed to find creation event for container {container_name}".format(
                container_name=container_name
            ),
            level=logging.ERROR,
        )
        # we will not raise an exception since this may be a common occurence
        # for long-lived containers and/or when we beign to use this functionality
        # unlike datadogEvent where we absolutely want valid tags and the exception will
        # never be invoked except in local testing (where we want to except), here this
        # could be invoked in a real environment and do not want to break the server


def datadogEvent_clusterLifecycle(cluster_name):
    """Effectively the same as datadogEvent_containerLifecycle, except for clusters.
    Read above.

    Args:
        cluster_name (str): Name of the string
    """
    deletion_date = time.time()

    event = _most_recent_by_tags([CLUSTER_NAME_F.format(cluster_name)])
    if event:
        if not CLUSTER_CREATION in event["tags"]:
            was_deletion = str(
                CLUSTER_DELETION in event["tags"] or CLUSTER_LIFECYCLE in event["tags"]
            )
            fractalLog(
                function="datadogEvent_clusterLifecycle",
                label=None,
                logs="Last event was not a creation for cluster {cluster_name}. Was it deletion? Answer: {was_deletion}.".format(
                    cluster_name=cluster_name, was_deletion=was_deletion
                ),
                level=logging.ERROR,
            )
        else:
            creation_date = event["date_happened"]

            runtime = str(deletion_date - creation_date)
            creation_date, deletion_date = str(creation_date), str(deletion_date)

            datadogEvent(
                title="Cluster Lifecycle Ended",
                text="Cluster {cluster_name} has been deleted, completing its lifecycle. Creation time was {creation_date} and deletion time was {deletion_date}. Change in time was {runtime}.".format(
                    cluster_name=cluster_name,
                    creation_date=creation_date,
                    deletion_date=deletion_date,
                    runtime=runtime,
                ),
                tags=[CLUSTER_LIFECYCLE, SUCCESS, CLUSTER_NAME_F.format(cluster_name=cluster_name)],
            )
    else:
        fractalLog(
            function="datadogEvent_clusterLifecycle",
            label=None,
            logs="Failed to find creation event for cluster {cluster_name}".format(
                cluster_name=cluster_name
            ),
            level=logging.ERROR,
        )


def _most_recent_by_tags(
    tag_by,
    check_event=lambda event: True,
    sort_by=lambda event: event["date_happened"],
    dt=3600,
    max_d=86400,
):
    """Helper method that will find the most recent (or some other order) event within the last
    max_d seconds (default one day) such that it had one of the tags in tag_by (it's an OR). If
    it finds nothing it will return None.

    Args:
        tag_by (List[str]): What tags to search for (OR of each).
        check_event(lambda event: bool, optional): a condition that MUST be true for this event to count
        sort_by (lambda event: int, optional): What to sort by, default is recency.
            Defaults to lambdaevent:event["date_happened"].
        dt (int, optional): How much to cinrement by on each try. Defaults to 3600.
        max_d (int, optional): Maximum amount of time in the past to check. Defaults to 86400.
    """
    d = dt

    end_time, start_time = time.time(), end_time - dt

    while d < max_d:
        events = api.Event.query(
            start=start_time,
            end=end_time,
            # sources ?
            tags=tag_by,
            unaggregated=True,
        )

        try:
            # https://docs.datadoghq.com/api/v1/events/
            return max((event for event in events if check_event(event)), key=sort_by)
        except:
            d += dt
            end_time, start_time = start_time, start_time - dt

    return None
