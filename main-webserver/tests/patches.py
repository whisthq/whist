"""Common patches used during testing."""

import uuid

from celery.result import AsyncResult


class CeleryResult(AsyncResult):
    def __init__(self):
        pass

    @property
    def id(self):
        return str(uuid.uuid4())


class Object:
    pass


class Patch:
    pass


def apply_async(*args, **kwargs):
    return CeleryResult()


def do_nothing(*args, **kwargs):
    pass


def function(**kwargs):
    def func(*_args, **_kwargs):

        if kwargs.get("raises"):
            raise kwargs["raises"]

        return kwargs.get("returns")

    return func
