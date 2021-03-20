# task_version in the SupportedAppImages/UserContainers has a default value of -1
# this constant exists for backcompat with pre-version pinning days. It allows
# the webserver to process tasks that don't have a task definition pinned yet
# by assuming a default of -1 in the payload body and handling the -1 case
# to mean run the latest task version
USE_LATEST_TASK_VERSION = -1
