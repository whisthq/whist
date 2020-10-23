import pytest

from app import app as _app


@pytest.fixture
def app():
    return _app
