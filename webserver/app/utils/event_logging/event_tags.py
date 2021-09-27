"""Logz takes in string tags that let us categorize different events
more easily than plain string searches
"""

MANDELBOX_CREATION = "mandelbox-creation"
MANDELBOX_ASSIGNMENT = "mandelbox-assignment"
MANDELBOX_DELETION = "mandelbox-deletion"

CLUSTER_CREATION = "cluster-creation"
CLUSTER_DELETION = "cluster-delection"

MANDELBOX_LIFECYCLE = "mandelbox-lifecycle"
CLUSTER_LIFECYCLE = "cluster-lifecycle"

_type_tags = {
    MANDELBOX_CREATION,
    MANDELBOX_DELETION,
    CLUSTER_CREATION,
    CLUSTER_DELETION,
    MANDELBOX_LIFECYCLE,
    CLUSTER_LIFECYCLE,
}

SUCCESS = "success"
FAILURE = "failure"

_status_tags = {SUCCESS, FAILURE}

# these need to be formatted
# do something like MANDELBOX_NAME.format(mandelbox_name="my_mandelbox")
# to get "mandelbox-name:my_mandelbox"
MANDELBOX_NAME = "mandelbox-name:{mandelbox_name}"
CLUSTER_NAME = "cluster-name:{cluster_name}"
MANDELBOX_USER = "mandelbox-user:{mandelbox_user}"


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
