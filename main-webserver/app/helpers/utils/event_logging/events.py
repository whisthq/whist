from app.helpers.utils.general.logs import fractal_logger


import logging
from time import time

# ones with F at the end must be formatted
from app.helpers.utils.event_logging.event_tags import (
    CONTAINER_ASSIGNMENT,
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
    valid_tags,
)
from app.helpers.utils.event_logging.event_text import to_text


# Logging programmatically will be advantageous over doing logs with filters exclusively in the event_logging console
# since programmaticaly we can be more precise and it's easier to learn how to use it (i.e. I don't need to read
# a ton of docs on event_logging filters/matches and other whatnot); it's also more flexible


def basic_logging_event(title, tags, text=""):
    """Logs a event_logging event to keep it in
    our main body of logs. We require tags to now allow people to make events that are not
    easily categorizeable. We require 2 tags, one for the event-type and one for the status.

    Args:
        title (str): The title of the event giving cursory information.
        text (str, optional): Body text of the event, giving detailed information. Defaults to "".
        tags (List[str]): Tags to tag the event with for filtering. Defaults to None.
    """

    if not valid_tags(tags):
        fractal_logger.error(
            "Tried to log a event_logging event with invalid tags: {tags}".format(tags=str(tags))
        )
        raise Exception("Event tags were not valid.")

    fmt_str = f"Logging event details:  Event {title} occurred with information {text} and metadata {tags}"
    fractal_logger.info(fmt_str)


# below are some commonly used events


def logged_event_for_logon(user_name):
    """Logs whether a user logged on.

    Args:
        user_name (str): Name of the user who just logged on.
    """
    basic_logging_event(
        title="User Logged On",
        text=to_text(username=user_name),
        tags=[LOGON, SUCCESS, USER_NAME_F.format(user_name=user_name)],
    )


def logged_event_container_prewarmed(
    container_name, cluster_name, username="unknown", time_taken="unknown"
):
    """Logs an event for container creation for future metric collection

    Args:
        container_name (str): The name of the container which was created.
            This is used to search in lifecycle.
        cluster_name (str): The cluster it was created in.
        username (str, optional): The username of the user this is created for.
        time_taken (int):  how long the prewarm took
    """

    tags = [
        CONTAINER_CREATION,
        SUCCESS,
        CONTAINER_NAME_F.format(container_name=container_name),
        CLUSTER_NAME_F.format(cluster_name=cluster_name),
    ]
    if username:
        tags.append(CONTAINER_USER_F.format(container_user=username))

    basic_logging_event(
        title="Created new Container",
        text=to_text(
            container_name=container_name,
            cluster_name=cluster_name,
            username=username,
            spinup_time=time_taken,
        ),
        tags=tags,
    )


def logged_event_container_assigned(
    container_name, cluster_name, username="unknown", time_taken="unknown"
):
    """Logs an event for container assignment.

    Args:
        container_name (str): The name of the container which was created.
            This is used to search in lifecycle.
        cluster_name (str): The cluster it was created in.
        username (str, optional): The username of the user this is created for.
        time_taken (int):  how long the operation took
    """

    tags = [
        CONTAINER_ASSIGNMENT,
        CONTAINER_LIFECYCLE,
        SUCCESS,
        CONTAINER_NAME_F.format(container_name=container_name),
        CLUSTER_NAME_F.format(cluster_name=cluster_name),
    ]
    if username:
        tags.append(CONTAINER_USER_F.format(container_user=username))

    basic_logging_event(
        title="Assigned new Container",
        text=to_text(
            container_name=container_name,
            cluster_name=cluster_name,
            username=username,
            spinup_time=time_taken,
        ),
        tags=tags,
    )


def logged_event_cluster_created(cluster_name, time_taken="unknown"):
    """Same as logged_event_container_prewarmed but for clusters.

    Args:
        cluster_name (str): The name of the cluster which was created.
            This is used to search in lifecycle.
        time_taken (int):  how long the operation took
    """
    basic_logging_event(
        title="Created new Cluster. Call took {time_taken} time.".format(time_taken=time_taken),
        text=to_text(cluster_name=cluster_name, spinup_time=time_taken),
        tags=[CLUSTER_CREATION, SUCCESS, CLUSTER_NAME_F.format(cluster_name=cluster_name)],
    )


def logged_event_user_logoff(user_name, lifecycle=False):
    """Logs whether a user logged off. Same as logon basically.

    Args:
        user_name (str): Name of the user who just logged off.
        lifecycle (bool, optional): Whether to do the lifecycle calculation, which involves us
            calculating how long the user was logged on, or to naively just say "logged off" with
            a timestamp.
    """
    if lifecycle:
        logged_event_user_lifecycle(user_name)
    else:
        basic_logging_event(
            title="User Logged Off",
            text=to_text(username=user_name),
            tags=[LOGOFF, SUCCESS, USER_NAME_F.format(user_name=user_name)],
        )


def logged_event_container_deleted(
    container_name, container_user, cluster_name, lifecycle=True, time_taken=None
):
    """Logs an event for the deletion of the container. It has a "naive" option and a
    lifecycle option. In the lifecycle option it will log a lifecycle type event instead
    of a deletion type event. This event will inform users of the length of time taken.
    In the case of the naive method (lifecycle=False) it will simply log that deletion
    without any calculation etc.

    Args:
        container_name (str): The name of the container that was deleted.
        cluster_name (str): The name of the cluster that the deleted container was in.
        lifecycle (bool, optional): Whether we want to measure time and return a lifecycle type
            event or prefer just to log the deletion itself naively. Defaults to False.'
        time_taken (int):  how long the operation took
    """
    if lifecycle:
        logged_event_container_lifecycle(
            container_name, container_user, cluster_name=cluster_name, time_taken=time_taken
        )
    else:
        basic_logging_event(
            title="Deleted Container",
            text=to_text(
                container_name=container_name,
                container_user=container_user,
                cluster_name=cluster_name,
                shutdown_time=time_taken,
            ),
            tags=[
                CONTAINER_DELETION,
                SUCCESS,
                CONTAINER_NAME_F.format(container_name=container_name),
                CLUSTER_NAME_F.format(cluster_name=cluster_name),
            ],
        )


def logged_event_cluster_deleted(cluster_name, lifecycle=False, time_taken="unknown"):
    """Same idea as logged_event_container_deleted but for clusters.

    Args:
        cluster_name (str): Name of the cluster deleted.
        lifecycle (bool, optional): Whether to do lifecycle type instead of naive. Defaults to False.
        time_taken (int):  how long the operation took
    """
    if lifecycle:
        logged_event_cluster_lifecycle(cluster_name, time_taken=time_taken)
    else:
        basic_logging_event(
            title="Deleted Cluster",
            text=to_text(cluster_name=cluster_name, shutdown_time=time_taken),
            tags=[CLUSTER_DELETION, SUCCESS, CLUSTER_NAME_F.format(cluster_name=cluster_name)],
        )


def logged_event_container_lifecycle(
    container_name, container_user, cluster_name="unknown", time_taken="unknown"
):
    """The goal is to tell the amount of time a container was up/down.

    This will log an event for the lifecycle of a container that is being shut down.
    It will search for the (most recent )previous event with tags corresponding to
    the creation of the container which is being deleted and then find the distance in time
    and put it into the body along with this event. This is meant to be filtered and then used
    for log analysis elsewhere.

    Note that as a @precondition, the container this is being invoked for must have been created
    while these event_logging logs were in effect, AND it must be deleted now. In the case where it is
    not being deleted then this will return false information (i.e. the container lived for more
    than this will log) without an error. If the container's creation was not found, then this
    will fractal_log an error with a corresponding message.

    Args:
        container_name (str): The name of the container whose lifecycle we are observing the end for.
    """
    deletion_date = time()

    deletion_date = str(deletion_date)

    basic_logging_event(
        title="Container Lifecycle Ended",
        text=to_text(
            container_name=container_name,
            end_date=deletion_date,
            username=container_user,
            cluster_name=cluster_name,
            shutdown_time=time_taken,
        ),
        tags=[
            CONTAINER_LIFECYCLE,
            SUCCESS,
            CONTAINER_NAME_F.format(container_name=container_name),
        ],
    )


def logged_event_cluster_lifecycle(cluster_name, time_taken="unknown"):
    """Effectively the same as logged_event_container_lifecycle, except for clusters.
    Read above.

    Args:
        cluster_name (str): Name of the string
        time_taken (int):  how long the operation took
    """
    deletion_date = time()
    deletion_date = str(deletion_date)

    basic_logging_event(
        title="Cluster Lifecycle Ended",
        text=to_text(
            cluster_name=cluster_name,
            end_date=deletion_date,
            shutdown_time=time_taken,
        ),
        tags=[CLUSTER_LIFECYCLE, SUCCESS, CLUSTER_NAME_F.format(cluster_name=cluster_name)],
    )


def logged_event_user_lifecycle(user_name):
    """Same as above but for log in/log off.

    Args:
        user_name (str): Name of the user who logged off and we'd like to find their previous log in
            or whatever.
    """
    end_date = time()
    end_date = str(end_date)

    basic_logging_event(
        title="User Lifecycle ended.",
        text=to_text(
            username=user_name,
            end_date=end_date,
        ),
        tags=[USER_LIFECYCLE, SUCCESS, USER_NAME_F.format(user_name=user_name)],
    )


def lifecycle_getter(_end_evt_name, _end_evt_type):
    # TODO: Implement this
    pass
