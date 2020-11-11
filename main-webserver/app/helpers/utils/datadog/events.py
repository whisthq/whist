from app import *
from app.helpers.utils.general.logs import fractalLog

from functools import reduce

import logging
from time import time
from datetime import timedelta
from flask import current_app
from datadog import initialize, api

# ones with F at the end must be formatted
from app.helpers.utils.datadog.event_tags import (
    CONTAINER_CREATION,
    CONTAINER_DELETION,
    CONTAINER_LIFECYCLE,
    CONTAINER_NAME as CONTAINER_NAME_F,  # container_name=
    CONTAINER_USER as CONTAINER_USER_F,
    CLUSTER_CREATION,
    CLUSTER_DELETION,
    CLUSTER_LIFECYCLE,
    CLUSTER_NAME as CLUSTER_NAME_F,  # cluster_name=
    LOGON,
    LOGOFF,
    USER_LIFECYCLE,
    USER_NAME as USER_NAME_F,
    SUCCESS,
    FAILURE,
    validTags,
)

# TODO make higher order functions or something to basically
# generate all the different variations of logs with some parameters

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


def datadogEvent_userLogon(user_name):
    """Logs whether a user logged on. This is used to help Leor (or whoever is maintaing
    the webserver now) to see whether users are using our service after they initially sign up.

    Args:
        user_name (str): Name of the user who just logged on.
    """
    datadogEvent(
        title="User Logged On",
        text="User {user_name} just logged on".format(user_name=user_name),
        tags=[LOGON, SUCCESS, USER_NAME_F.format(user_name=user_name)],
    )


def datadogEvent_containerCreate(
    container_name, cluster_name, username="unknown", time_taken="unknown"
):
    """Logs an event for container creation. This is necessary for lifecycle events to be
    able to work so that they can find the time that the deleted container was created.

    Args:
        container_name (str): The name of the container which was created.
            This is used to search in lifecycle.
        cluster_name (str): The cluster it was created in.
        username (str, optional): The username of the user this is created for.
        time_taken
    """

    tags = [
        CONTAINER_CREATION,
        SUCCESS,
        CONTAINER_NAME_F.format(container_name=container_name),
        CLUSTER_NAME_F.format(cluster_name=cluster_name),
    ]
    if username:
        tags.append(CONTAINER_USER_F.format(container_user=username))

    datadogEvent(
        title="Created new Container",
        text="Container {container_name} in cluster {cluster_name} for user {username}. Call took {time_taken} time.".format(
            container_name=container_name,
            cluster_name=cluster_name,
            username=username,
            time_taken=time_taken,
        ),
        tags=tags,
    )


def datadogEvent_clusterCreate(cluster_name, time_taken="unknown"):
    """Same as datadogEvent_containerCreate but for clusters.

    Args:
        cluster_name (str): The name of the cluster which was created.
            This is used to search in lifecycle.
    """
    datadogEvent(
        title="Created new Cluster. Call took {time_taken} time.".format(time_taken=time_taken),
        text="Cluster {cluster_name}".format(cluster_name=cluster_name),
        tags=[CLUSTER_CREATION, SUCCESS, CLUSTER_NAME_F.format(cluster_name=cluster_name)],
    )


def datadogEvent_userLogoff(user_name, lifecycle=False):
    """Logs whether a user logged off. Same as logon basically.

    Args:
        user_name (str): Name of the user who just logged off.
        lifecycle (bool, optional): Whether to do the lifecycle calculation, which involves us
            calculating how long the user was logged on, or to naively just say "logged off" with
            a timestamp.
    """
    if lifecycle:
        datadogEvent_userLifecycle(user_name)
    else:
        datadogEvent(
            title="User Logged Off",
            text="User {user_name} just logged off".format(user_name=user_name),
            tags=[LOGOFF, SUCCESS, USER_NAME_F.format(user_name=user_name)],
        )


def datadogEvent_containerDelete(container_name, cluster_name, lifecycle=False, time_taken=None):
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
        datadogEvent_containerLifecycle(
            container_name, cluster_name=cluster_name, time_taken=time_taken
        )
    else:
        datadogEvent(
            title="Deleted Container",
            text="Container {container_name} in cluster {container_cluster}. Call took {time_taken} time.".format(
                container_name=container_name, cluster_name=cluster_name, time_taken=time_taken
            ),
            tags=[
                CONTAINER_DELETION,
                SUCCESS,
                CONTAINER_NAME_F.format(container_name=container_name),
                CLUSTER_NAME_F.format(cluster_name=cluster_name),
            ],
        )


def datadogEvent_clusterDelete(cluster_name, lifecycle=False, time_taken="unknown"):
    """Same idea as datadogEvent_containerDelete but for clusters.

    Args:
        cluster_name (str): Name of the cluster deleted.
        lifecycle (bool, optional): Whether to do lifecycle type instead of naive. Defaults to False.
    """
    if lifecycle:
        datadogEvent_clusterLifecycle(cluster_name, time_taken=time_taken)
    else:
        datadogEvent(
            title="Deleted Cluster",
            text="Cluster {cluster_name}. Call took {time_taken} time.".format(
                cluster_name=cluster_name, time_taken=time_taken
            ),
            tags=[CLUSTER_DELETION, SUCCESS, CLUSTER_NAME_F.format(cluster_name=cluster_name)],
        )


def datadogEvent_containerLifecycle(container_name, cluster_name="unknown", time_taken="unknown"):
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
    deletion_date = time()

    event = _most_recent_by_tags(
        [CONTAINER_NAME_F.format(container_name=container_name)],
        # TODO in the future maybe use check_event
        # right now we want to see if the first event is a deletion since that it's better not
        # to log a wrong event
    )
    if event:
        tags = event["tags"]
        container_user = reduce(
            # splitting with second param 1 means to split along the first : (arn has :)
            lambda acc, tag: acc if not "container-user" in tags else tag.split(":", 1)[1],
            tags,
            "unknown",
        )

        if not CONTAINER_CREATION in tags:
            was_deletion = str(CONTAINER_DELETION in tags or CONTAINER_LIFECYCLE in tags)
            fractalLog(
                function="datadogEvent_containerLifecycle",
                label=None,
                logs="Last event was not a creation for container {container_name}. Was it deletion? Answer: {was_deletion}. The user was {cointainer_user}.".format(
                    container_name=container_name,
                    was_deletion=was_deletion,
                    container_user=container_user,
                ),
                level=logging.ERROR,
            )
        else:
            creation_date = event["date_happened"]

            # find everything as a string so we can embedd it in a format string
            runtime = str(timedelta(seconds=(deletion_date - creation_date)))
            creation_date, deletion_date = str(creation_date), str(deletion_date)

            datadogEvent(
                title="Container Lifecycle Ended",
                text="Container {container_name} has been deleted, completing its lifecycle. Creation time was {creation_date} and deletion time was {deletion_date}. Change in time was {runtime}. Ther user was {container_user}".format(
                    container_name=container_name,
                    creation_date=creation_date,
                    deletion_date=deletion_date,
                    runtime=runtime,
                    container_user=container_user,
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


def datadogEvent_clusterLifecycle(cluster_name, time_taken="unknown"):
    """Effectively the same as datadogEvent_containerLifecycle, except for clusters.
    Read above.

    Args:
        cluster_name (str): Name of the string
    """
    deletion_date = time()

    event = _most_recent_by_tags([CLUSTER_NAME_F.format(cluster_name=cluster_name)])
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

            runtime = str(timedelta(seconds=(deletion_date - creation_date)))
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


def datadogEvent_userLifecycle(user_name):
    """Same as above but for log in/log off.

    Args:
        user_name (str): Name of the user who logged off and we'd like to find their previous log in
            or whatever.
    """
    end_date = time()

    event = _most_recent_by_tags([USER_NAME_F.format(user_name=user_name)])
    if event:
        if not LOGON in event["tags"]:
            was_logoff = str(LOGOFF in event["tags"] or USER_LIFECYCLE in event["tags"])
            fractalLog(
                function="datadogEvent_userLifecycle",
                label=None,
                logs="Last event was not a log on for user {user_name}. Was it log off? Answer: {was_logoff}.".format(
                    user_name=user_name, was_logoff=was_logoff
                ),
                level=logging.ERROR,
            )
        else:
            start_date = event["date_happened"]

            runtime = str(timedelta(seconds=(end_date - start_date)))
            start_date, end_date = str(start_date), str(end_date)

            datadogEvent(
                title="User Lifcycle ended.",
                text="User {user_name} logged off, completing his/her lifecycle. Log on time was {start_date} and log off time was {end_date}. Change in time was {runtime}.".format(
                    user_name=user_name,
                    start_date=start_date,
                    end_date=end_date,
                    runtime=runtime,
                ),
                tags=[USER_LIFECYCLE, SUCCESS, USER_NAME_F.format(user_name=user_name)],
            )
    else:
        fractalLog(
            function="datadogEvent_userLifecycle",
            label=None,
            logs="Failed to find logon event for user {user_name}".format(user_name=user_name),
            level=logging.ERROR,
        )


def _most_recent_by_tags(
    tag_by,
    check_event=lambda event: True,
    sort_by=lambda event: event["date_happened"],
    dt=3600,  # 60 sec * 60 min = 1 hour
    max_d=86400,  # 86400 seconds = 1 hr * 24 = 1 day
    init=True,
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
    if init:
        initDatadog()

    d = dt

    end_time = time()
    start_time = end_time - dt

    while d < max_d:
        events = api.Event.query(
            start=start_time,
            end=end_time,
            # sources ?
            tags=tag_by,
            unaggregated=True,
        )[
            "events"
        ]  # bruh why this convention it sucks

        try:
            # https://docs.datadoghq.com/api/v1/events/
            return max((event for event in events if check_event(event)), key=sort_by)
        except:
            d += dt
            end_time, start_time = start_time, start_time - dt

    return None
