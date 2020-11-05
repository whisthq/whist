"""Datadog takes in string tags that let us categorize different events into categories depending
on what they correspond to (which makes analysis way easier). Here are some basic tags that are commonly
used by our datadog loggers. Please use these constants to keep our code clean in a growing codebase.
"""

CONTAINER_CREATION = "container-creation"
CONTAINER_DELETION = "container-deletion"

CLUSTER_CREATION = "cluster-creation"
CLUSTER_DELETION = "cluster-delection"

CONTAINER_LIFECYCLE = "container-lifecycle"
CLUSTER_LIFECYCLE = "cluster-lifecycle"

SUCCESS = "success"
FAILURE = "failure"

# these need to be formatted
# do something like CONTAINER_NAME.format(container_name="my_container")
# to get "container-name:my_container"
CONTAINER_NAME = "container-name:{container_name}"
CLUSTER_NAME = "cluster-name:{cluster_name}"
