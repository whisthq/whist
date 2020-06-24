import sys
import os
import tempfile
import pytest
import requests
from dotenv import load_dotenv

load_dotenv()
SERVER_URL = "https://" + os.getenv("HEROKU_APP_NAME") + ".herokuapp.com"


def fetchvms(username):
    return requests.post((SERVER_URL + "/user/login"), json=dict(username=username))


def test_fetchvms():
    print(SERVER_URL)
    email = "isa.zheng@gmail.com"
    rv = fetchvms(email)
    assert rv.status_code == 200
