import os
import pytest
import requests

from app import app as _app
from app.helpers.utils.general.logs import fractalLog
from tests.constants.resources import SERVER_URL


@pytest.fixture
def app():
    return _app


@pytest.fixture
def input_token():
    resp = requests.post(
        (SERVER_URL + "/account/login"),
        json=dict(
            username=os.getenv("DASHBOARD_USERNAME"), password=os.getenv("DASHBOARD_PASSWORD"),
        ),
    )

    return resp.json()["access_token"]


@pytest.fixture
def admin_token():
    resp = requests.post(
        (SERVER_URL + "/admin/login"),
        json=dict(
            username=os.getenv("DASHBOARD_USERNAME"), password=os.getenv("DASHBOARD_PASSWORD")
        ),
    )
    return resp.json()["access_token"]
