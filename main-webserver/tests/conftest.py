import pytest
import requests
import os
import logging
import time

SERVER_URL = "https://" + os.getenv("HEROKU_APP_NAME") + ".herokuapp.com" if os.getenv("HEROKU_APP_NAME") else "http://localhost:5000"

LOGGER = logging.getLogger(__name__)

def getVersions():
    return requests.get((SERVER_URL + "/version"))

@pytest.fixture(scope="session")
def setup(request):
    # only wait for deployment if not local
    if os.getenv("HEROKU_APP_NAME"):
        LOGGER.info("Waiting for server to deploy...")
        time.sleep(360)
        i = 1
        while getVersions().status_code != 200:
            time.sleep(10)
            i += 1
            LOGGER.info(str(i) + " times pinging server")
        LOGGER.info("Server deployed! Tests starting now.")
    return

@pytest.fixture
def input_token():
    resp = requests.post(
        (SERVER_URL + "/account/login"),
        json=dict(username="whatever", password="whatever"),
    )
    return resp.json()["access_token"]
