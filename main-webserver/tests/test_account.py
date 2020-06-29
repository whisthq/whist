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


def test_login(input_token):
    register(
        "fakefake@delete.com",
        "password",
        "Delete Me",
        "Two men walk into a bar. Knock knock.",
    )
    resp = login("fakefake@delete.com", "password")
    delete("fakefake@delete.com", input_token)
    assert resp.json()["verified"]
    resp = login("support@fakecomputers.com", "asdf")
    assert not resp.json()["verified"]


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
    register(
        "fakefake@delete.com",
        "password",
        "Delete Me",
        "Two men walk into a bar. Knock knock.",
    )
    resp = requests.post(
        (SERVER_URL + "/account/fetchUser"),
        json={"username": "fakefake@delete.com"},
        headers={"Authorization": "Bearer " + input_token},
    )
    success = resp.json()["user"] is not None
    delete("fakefake@delete.com", input_token)
    assert success


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
    delete("fakefake@delete.com", input_token)
    resp = requests.post(
        (SERVER_URL + "/account/fetchUser"),
        json={"username": "fakefake@delete.com"},
        headers={"Authorization": "Bearer " + input_token},
    )
    assert resp.json()["user"] is None


def adminLogin(username, password):
    return requests.post(
        (SERVER_URL + "/admin/login"), json=dict(username=username, password=password)
    )


def test_adminLogin():
    resp = adminLogin("example@example.com", "password")
    assert resp.status_code == 422
    resp = adminLogin(os.getenv("DASHBOARD_USERNAME"), os.getenv("DASHBOARD_PASSWORD"))
    assert resp.status_code == 200

def lookup(username):
    return requests.post((SERVER_URL + "/account/lookup"),
    json={"username": username})

def test_lookup(input_token):
    register(
        "testlookup@example.com",
        "password",
        "Test Lookup",
        "Two men walk into a bar. Knock knock.",
    )
    resp = lookup("testlookup@example.com")
    assert resp.json()["exists"]

    delete("testlookup@example.com", input_token)

    resp = lookup("doesnotexist@example.com")
    assert not resp.json()["exists"]
