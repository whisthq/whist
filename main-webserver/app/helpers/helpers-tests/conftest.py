import pytest

from app.factory import create_app


@pytest.fixture
def app():
    _app = create_app()
    _app.testing = True

    return _app
