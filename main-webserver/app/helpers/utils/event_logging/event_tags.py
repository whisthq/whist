"""Logz takes in string tags that let us categorize different events
more easily than plain string searches
"""

CONTAINER_CREATION = "container-creation"
CONTAINER_ASSIGNMENT = "container-assignment"
CONTAINER_DELETION = "container-deletion"

CLUSTER_CREATION = "cluster-creation"
CLUSTER_DELETION = "cluster-delection"

CONTAINER_LIFECYCLE = "container-lifecycle"
CLUSTER_LIFECYCLE = "cluster-lifecycle"

_type_tags = {
    CONTAINER_CREATION,
    CONTAINER_DELETION,
    CLUSTER_CREATION,
    CLUSTER_DELETION,
    CONTAINER_LIFECYCLE,
    CLUSTER_LIFECYCLE,
}

SUCCESS = "success"
FAILURE = "failure"

_status_tags = {SUCCESS, FAILURE}

# these need to be formatted
# do something like CONTAINER_NAME.format(container_name="my_container")
# to get "container-name:my_container"
CONTAINER_NAME = "container-name:{container_name}"
CLUSTER_NAME = "cluster-name:{cluster_name}"
CONTAINER_USER = "container-user:{container_user}"


def valid_tags(tags):
    """Check whether the given tags are valid in that
    they contain the minimum requirements to be
    logged to event_logging. There should not be more than one of these.

    Args:
        tags (List[str]): some tags that someone wants to put in some event_logging event

    Returns:
        boolean: [description]
    """
    has_status, has_type = 0, 0
    for tag in tags:
        if tag in _status_tags:
            has_status += 1
        elif tag in _type_tags:
            has_type += 1

    return has_status == 1 and has_type == 1
