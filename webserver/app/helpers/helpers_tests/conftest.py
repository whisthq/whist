from flask import Flask
import pytest

from app.factory import create_app


@pytest.fixture
def app() -> Flask:
    _app = create_app(testing=True)

    return _app
