import pytest
import requests

from requests import ConnectionError
from app.helpers.utils.aws.base_ecs_client import ECSClient
from app.exceptions import StartValueException
from app.celery.aws_ecs_creation import (
    _pass_start_values_to_instance,
    _clean_tasks_and_create_new_container,
    _mount_cloud_storage,
    _assign_container,
)
from ..patches import function, Object


def test_pass_dpi_failure(user, container, monkeypatch):
    """Test that `_pass_start_values_to_instance` handles errors correctly.

    This tests how `_pass_start_values_to_instance` behaves when a ConnectionError
    is raised, simulating when it fails to connect to the ECS Host Service or receives
    a bad response.
    """
    monkeypatch.setattr(requests, "put", function(raises=ConnectionError))
    with container() as c:
        with pytest.raises(StartValueException):
            _pass_start_values_to_instance(c)


def test_clean(user, container, monkeypatch):
    """Test that `_clean_tasks_and_create_new_container` doesn't have any errors.

    This tests how `_clean_tasks_and_create_new_container` that there are no errors
    when cleaning up the db. It patches the assign_container function so it doesn't
    actually get called.
    """

    monkeypatch.setattr("app.celery.aws_ecs_creation.assign_container", function(returns=True))
    monkeypatch.setattr(ECSClient, "check_if_done", function(returns=True))
    with container() as c:
        _clean_tasks_and_create_new_container(c, 0, "", 0)
