import sys
import os
import pytest
import requests
from dotenv import load_dotenv

load_dotenv()
SERVER_URL = (
    "https://main-webserver-pr-67" + ".herokuapp.com"
    if os.getenv("HEROKU_APP_NAME")
    else "http://localhost:5000"
)


@pytest.fixture
def input_token():
    resp = requests.post(
        (SERVER_URL + "/account/login"),
        json=dict(username="whatever", password="whatever"),
    )
    return resp.json()["access_token"]
