"""Pytest fixtures that allow for easy moto integration."""

import pytest

from moto import mock_iam


@pytest.fixture
def iam(pytestconfig):
    if pytestconfig.getoption("mock_aws"):
        with mock_iam():
            yield
    else:
        yield
