from app.exceptions import _FractalError


class InvalidTaskDefinition(Exception):
    pass


class ContainerNotAvailableError(_FractalError):
    params = ("task_arn", "num_tries")
    message = "AWS task {task_arn} failed to become available after {num_tries}."
