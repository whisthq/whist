import sys
import os
import pytest
import requests
from dotenv import load_dotenv

load_dotenv()
SERVER_URL = (
    "https://" + os.getenv("HEROKU_APP_NAME") + ".herokuapp.com"
    if os.getenv("HEROKU_APP_NAME")
    else "http://localhost:5000"
)


def login(username, password):
    return requests.post(
        (SERVER_URL + "/account/login"), json=dict(username=username, password=password)
    )


def test_login():
    resp = login("example@example.com", "password")
    json_data = resp.json()
    assert resp.status_code == 200


def register(username, password, name, feedback):
    return requests.post(
        (SERVER_URL + "/account/register"),
        json={
            "username": username,
            "password": password,
            "name": name,
            "feedback": feedback,
        },
    )


def test_register(input_token):
    resp = register(
        "fakefake@delete.com",
        "password",
        "Delete Me",
        "Two men walk into a bar. Knock knock.",
    )
    if resp.status_code == 200:  # Delete test account if successful
        delete("fakefake@delete.com", input_token)
    assert resp.status_code == 200


def delete(username, authToken):
    return requests.post(
        (SERVER_URL + "/account/delete"),
        json={"username": username},
        headers={"Authorization": "Bearer " + authToken},
    )


def test_delete(input_token):
    """Creates a test user and deletes it
    """
    register(
        "fakefake@delete.com",
        "password",
        "Delete Me",
        "Two men walk into a bar. Knock knock.",
    )
    resp = delete("fakefake@delete.com", input_token)
    assert resp.status_code == 200
