from app.exceptions import _FractalError


class InvalidAppId(Exception):
    """
    Raised when the user provides an invalid app id.
    """


class InvalidArguments(Exception):
    """
    Raised when the user provides bad arguments to a celery task
    """


class InvalidCluster(Exception):
    """
    Raised when a cluster is not found in the database
    """


class ContainerNotAvailableError(_FractalError):
    params = ("task_arn", "num_tries")
    message = "AWS task {task_arn} failed to become available after {num_tries}."
