import sys
import os
import tempfile
import pytest
import requests

SERVER_URL = "https://" + os.getenv("HEROKU_APP_NAME") + ".herokuapp.com" if os.getenv("HEROKU_APP_NAME") else "http://localhost:5000"

def login(username, password):
    return requests.post((SERVER_URL + '/account/login'), json=dict(
        username=username,
        password=password
    ))

def fetchvms(username):
    return requests.post((SERVER_URL + "/user/login"), json=dict(
        username=username
    ))

def test_fetchvms():
    print(os.getenv("HEROKU_APP_NAME"))
    email = 'isa.zheng@gmail.com'
    rv = fetchvms(email)
    assert rv.status_code == 200

def test_login():
    email = 'example@example.com'
    rv = login(email, 'password')
    json_data = rv.json()
    print(json_data)
    assert rv.status_code == 200
