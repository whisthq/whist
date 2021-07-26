from time import time

from app.helpers.utils.general.logs import fractal_logger

# ones with F at the end must be formatted
from app.helpers.utils.event_logging.event_tags import (
    MANDELBOX_ASSIGNMENT,
    MANDELBOX_CREATION,
    MANDELBOX_DELETION,
    MANDELBOX_LIFECYCLE,
    MANDELBOX_NAME as MANDELBOX_NAME_F,  # mandelbox_name=
    MANDELBOX_USER as MANDELBOX_USER_F,
    CLUSTER_CREATION,
    CLUSTER_DELETION,
    CLUSTER_LIFECYCLE,
    CLUSTER_NAME as CLUSTER_NAME_F,  # cluster_name=
    SUCCESS,
    valid_tags,
)
from app.helpers.utils.event_logging.event_text import to_text


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

    fmt_str = f"Logging event details:  Event {title}\
    occurred with information {text} and metadata {tags}"
    fractal_logger.info(fmt_str)


# below are some commonly used events
def logged_event_mandelbox_prewarmed(
    mandelbox_name, cluster_name, username="unknown", time_taken="unknown"
):
    """Logs an event for mandelbox creation for future metric collection

    Args:
        mandelbox_name (str): The name of the mandelbox which was created.
            This is used to search in lifecycle.
        cluster_name (str): The cluster it was created in.
        username (str, optional): The username of the user this is created for.
        time_taken (int):  how long the prewarm took
    """

    tags = [
        MANDELBOX_CREATION,
        SUCCESS,
        MANDELBOX_NAME_F.format(mandelbox_name=mandelbox_name),
        CLUSTER_NAME_F.format(cluster_name=cluster_name),
    ]
    if username:
        tags.append(MANDELBOX_USER_F.format(mandelbox_user=username))

    basic_logging_event(
        title="Created new mandelbox",
        text=to_text(
            mandelbox_name=mandelbox_name,
            cluster_name=cluster_name,
            username=username,
            spinup_time=time_taken,
        ),
        tags=tags,
    )


def logged_event_mandelbox_assigned(
    mandelbox_name, cluster_name, username="unknown", time_taken="unknown"
):
    """Logs an event for mandelbox assignment.

    Args:
        mandelbox_name (str): The name of the mandelbox which was created.
            This is used to search in lifecycle.
        cluster_name (str): The cluster it was created in.
        username (str, optional): The username of the user this is created for.
        time_taken (int):  how long the operation took
    """

    tags = [
        MANDELBOX_ASSIGNMENT,
        MANDELBOX_LIFECYCLE,
        SUCCESS,
        MANDELBOX_NAME_F.format(mandelbox_name=mandelbox_name),
        CLUSTER_NAME_F.format(cluster_name=cluster_name),
    ]
    if username:
        tags.append(MANDELBOX_USER_F.format(mandelbox_user=username))

    basic_logging_event(
        title="Assigned new mandelbox",
        text=to_text(
            mandelbox_name=mandelbox_name,
            cluster_name=cluster_name,
            username=username,
            spinup_time=time_taken,
        ),
        tags=tags,
    )


def logged_event_cluster_created(cluster_name, time_taken="unknown"):
    """Same as logged_event_mandelbox_prewarmed but for clusters.

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


def logged_event_mandelbox_deleted(
    mandelbox_name, mandelbox_user, cluster_name, lifecycle=True, time_taken=None
):
    """Logs an event for the deletion of the mandelbox. It has a "naive" option and a
    lifecycle option. In the lifecycle option it will log a lifecycle type event instead
    of a deletion type event. This event will inform users of the length of time taken.
    In the case of the naive method (lifecycle=False) it will simply log that deletion
    without any calculation etc.

    Args:
        mandelbox_name (str): The name of the mandelbox that was deleted.
        cluster_name (str): The name of the cluster that the deleted mandelbox was in.
        lifecycle (bool, optional): Whether we want to measure time and return a lifecycle type
            event or prefer just to log the deletion itself naively. Defaults to False.'
        time_taken (int):  how long the operation took
    """
    if lifecycle:
        logged_event_mandelbox_lifecycle(
            mandelbox_name, mandelbox_user, cluster_name=cluster_name, time_taken=time_taken
        )
    else:
        basic_logging_event(
            title="Deleted mandelbox",
            text=to_text(
                mandelbox_name=mandelbox_name,
                mandelbox_user=mandelbox_user,
                cluster_name=cluster_name,
                shutdown_time=time_taken,
            ),
            tags=[
                MANDELBOX_DELETION,
                SUCCESS,
                MANDELBOX_NAME_F.format(mandelbox_name=mandelbox_name),
                CLUSTER_NAME_F.format(cluster_name=cluster_name),
            ],
        )


def logged_event_cluster_deleted(cluster_name, lifecycle=False, time_taken="unknown"):
    """Same idea as logged_event_mandelbox_deleted but for clusters.

    Args:
        cluster_name (str): Name of the cluster deleted.
        lifecycle (bool, optional): Whether to do lifecycle type instead of naive.
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


def logged_event_mandelbox_lifecycle(
    mandelbox_name, mandelbox_user, cluster_name="unknown", time_taken="unknown"
):
    """The goal is to tell the amount of time a mandelbox was up/down.

    This will log an event for the lifecycle of a mandelbox that is being shut down.
    It will search for the (most recent )previous event with tags corresponding to
    the creation of the mandelbox which is being deleted and then find the distance in time
    and put it into the body along with this event. This is meant to be filtered and then used
    for log analysis elsewhere.

    Args:
        mandelbox_name (str): The name of the mandelbox whose lifecycle we are observing the end of.
    """
    deletion_date = time()

    deletion_date = str(deletion_date)

    basic_logging_event(
        title="Mandelbox Lifecycle Ended",
        text=to_text(
            mandelbox_name=mandelbox_name,
            end_date=deletion_date,
            username=mandelbox_user,
            cluster_name=cluster_name,
            shutdown_time=time_taken,
        ),
        tags=[
            MANDELBOX_LIFECYCLE,
            SUCCESS,
            MANDELBOX_NAME_F.format(mandelbox_name=mandelbox_name),
        ],
    )


def logged_event_cluster_lifecycle(cluster_name, time_taken="unknown"):
    """Effectively the same as logged_event_mandelbox_lifecycle, except for clusters.
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


def lifecycle_getter(_end_evt_name, _end_evt_type):
    # TODO: Implement this
    pass
