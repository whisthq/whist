import sys
import os
import pytest
import requests
from dotenv import load_dotenv
import time

load_dotenv()
SERVER_URL = (
    "https://main-webserver-pr-70" + ".herokuapp.com"
    if os.getenv("CI")
    else "http://localhost:5000"
)

def test_login(input_token):
    username = "testlogin@example.com"
    try:
        pytest.helpers.register_user(
            username,
            "password",
            "Delete Me",
            "Two men walk into a bar. Knock knock.",
        )
        resp = pytest.helpers.login(username, "password")
        pytest.helpers.deleteUser(username, input_token)
        assert resp.json()["verified"]
        resp = pytest.helpers.login("support@fakecomputers.com", "asdf")
        assert not resp.json()["verified"]
    except:
        pytest.helpers.deleteUser(username, input_token)


def test_register_user(input_token):
    username = 'testRegistered@example.com'
    try:
        pytest.helpers.register_user(
            username,
            "password",
            "Delete Me",
            "Two men walk into a bar. Knock knock.",
        )
        resp = requests.post(
            (SERVER_URL + "/account/fetchUser"),
            json={"username": username},
            headers={"Authorization": "Bearer " + input_token},
        )
        success = resp.json()["user"] is not None
        pytest.helpers.deleteUser(username, input_token)
        assert success
    except:
        pytest.helpers.deleteUser(username, input_token)


def test_delete(input_token):
    """Creates a test user and deletes it
    """
    username = "testDelete@example.com"
    try:
        pytest.helpers.register_user(
            username,
            "password",
            "Delete Me",
            "Two men walk into a bar. Knock knock.",
        )
        pytest.helpers.deleteUser(username, input_token)
        resp = requests.post(
            (SERVER_URL + "/account/fetchUser"),
            json={"username": username},
            headers={"Authorization": "Bearer " + input_token},
        )
        assert resp.json()["user"] is None
    except:
        pytest.helpers.deleteUser(username, input_token)


def test_adminLogin():
    resp = pytest.helpers.adminLogin("example@example.com", "password")
    assert resp.status_code == 422
    resp = pytest.helpers.adminLogin(os.getenv("DASHBOARD_USERNAME"), os.getenv("DASHBOARD_PASSWORD"))
    assert resp.status_code == 200


def test_lookup(input_token):
    username = "testlookup@example.com"
    try:
        resp = pytest.helpers.lookup(username)
        assert not resp.json()["exists"]

        pytest.helpers.register_user(
            username,
            "password",
            "Test Lookup",
            "Two men walk into a bar. Knock knock.",
        )
        resp = pytest.helpers.lookup(username)
        assert resp.json()["exists"]

        pytest.helpers.deleteUser(username, input_token)

        resp = pytest.helpers.lookup("doesnotexist@example.com")
        assert not resp.json()["exists"]
    except:
        pytest.helpers.deleteUser(username, input_token)


def test_checkVerified(input_token):
    username = "testCheckVerified@example.com"
    try:
        resp = pytest.helpers.register_user(
            username, "password", "Test CheckVerified", "Here is some feedback.",
        )
        token = resp.json()["token"]
        resp = pytest.helpers.checkVerified(username)
        assert not resp.json()["verified"]
        resp = pytest.helpers.makeVerified(username, token, input_token)
        assert resp.json()["verified"]
        resp = pytest.helpers.checkVerified(username)
        assert resp.json()["verified"]
        pytest.helpers.deleteUser(username, input_token)
    except:
        pytest.helpers.deleteUser(username, input_token)


def test_reset(input_token):
    username = "testReset@example.com"
    try:
        pytest.helpers.register_user(
            username, "password", "Test Reset", "Here is some feedback.",
        )
        new_password = "new_password123"
        resp = pytest.helpers.login(username, new_password)
        assert not resp.json()["verified"]
        resp = pytest.helpers.reset(username, new_password)
        assert resp.json()["status"] == 200
        resp = pytest.helpers.login(username, new_password)
        assert resp.json()["verified"]
        pytest.helpers.deleteUser(username, input_token)
    except:
        pytest.helpers.deleteUser(username, input_token)


def test_fetchDisks(input_token):
    username = "testDisks@example.com"
    try:
        pytest.helpers.register_user(
            username, "password", "Test fetchDisks", "Some more feedback.",
        )

        resp = pytest.helpers.fetchDisks(username)
        assert len(resp["disks"]) == 0

        resp = pytest.helpers.createFromImage(username, input_token)
        assert resp.status_code == 202
        id = resp.json()["ID"]
        status = "PENDING"
        while status == "PENDING" or status == "STARTED":
            time.sleep(5)
            status = pytest.helpers.getStatus(id)["state"]
        assert status == "SUCCESS"
        disk = pytest.helpers.getStatus(id)["output"]

        resp = pytest.helpers.fetchDisks(username)
        assert len(resp["disks"]) > 0
        assert disk["disk_name"] in list(map(lambda d: d["disk_name"], resp["disks"]))

        pytest.helpers.deleteDisks(username, input_token)

        pytest.helpers.deleteUser(username, input_token)
    except:
        pytest.helpers.deleteDisks(username, input_token)
        pytest.helpers.deleteUser(username, input_token)
