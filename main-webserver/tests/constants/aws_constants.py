from app.constants.aws_constants import USE_LATEST_TASK_VERSION

# this is a constant used in tests to test that the db changes properly
# we use -2 because -1 is taken by USE_LATEST_TASK_VERSION. We can't pick
# an arbitrary positive number because that could be the current pinned task
# version. hence, -2 is chosen. No task should ever have a pinned task
# definition of -2 in the db (outside of transient test status)
DUMMY_TASK_VERSION = -2

assert USE_LATEST_TASK_VERSION != DUMMY_TASK_VERSION
