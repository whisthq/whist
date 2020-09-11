"""ECS management endpoint test fixtures."""

import pytest

from .session import Session


@pytest.fixture
def session():
    return Session()
