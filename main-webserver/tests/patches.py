"""Common patches used during testing."""

import uuid

from celery.result import AsyncResult


class CeleryResult(AsyncResult):
    def __init__(self):
        pass

    @property
    def id(self):
        return str(uuid.uuid4())


def apply_async(*args, **kwargs):
    return CeleryResult()


def do_nothing(*args, **kwargs):
    pass
