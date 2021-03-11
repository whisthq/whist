import pytest
import requests

from requests import ConnectionError
from app.celery.aws_ecs_creation import (
    _pass_start_values_to_instance,
    StartValueException,
)

from ..patches import function, Object


def test_pass_dpi_failure(user, container, monkeypatch):
    monkeypatch.setattr(requests, "put", function(raises=ConnectionError))
    with container() as c:
        with pytest.raises(StartValueException):
            _pass_start_values_to_instance(c, "whatever")
