import pytest
import requests

from requests import ConnectionError
from app.exceptions import StartValueException
from app.celery.aws_ecs_creation import (
    _pass_start_values_to_instance,
)

from ..patches import function, Object


def test_pass_dpi_failure(user, container, monkeypatch):
    """Test that `_pass_start_values_to_instance` handles errors correctly.

    This tests how `_pass_start_values_to_instance` behaves when a ConnectionError
    is raised, simulating when it fails to connect to the ECS Host Service/
    """
    monkeypatch.setattr(requests, "put", function(raises=ConnectionError))
    with container() as c:
        with pytest.raises(StartValueException):
            _pass_start_values_to_instance(c)
