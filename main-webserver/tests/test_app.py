import sys
import os
import tempfile
import pytest
import requests
import logging

SERVER_URL = "https://" + os.getenv("HEROKU_APP_NAME") + ".herokuapp.com" if os.getenv("HEROKU_APP_NAME") else "http://localhost:5000"

LOGGER = logging.getLogger(__name__)



def fetchvms(username):
    return requests.post((SERVER_URL + "/user/login"), json=dict(username=username))


def test_fetchvms(setup):
    email = 'isa.zheng@gmail.com'
    rv = fetchvms(email)
    assert rv.status_code == 200

def test_login(setup):
    email = 'example@example.com'
    rv = login(email, 'password')
    json_data = rv.json()
    assert rv.status_code == 200
