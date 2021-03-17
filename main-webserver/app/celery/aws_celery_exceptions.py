from app.exceptions import _FractalError


class InvalidAppId(Exception):
    """
    Raised when the user provides an invalid app id.
    """


class InvalidTaskDefinition(Exception):
    """
    Raised when the user provides an invalid task definition.
    """


class ContainerNotAvailableError(_FractalError):
    params = ("task_arn", "num_tries")
    message = "AWS task {task_arn} failed to become available after {num_tries}."
