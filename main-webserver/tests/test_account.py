import sys
import os
import pytest
import requests
from dotenv import load_dotenv

load_dotenv()
SERVER_URL = (
    "https://main-webserver-pr-70" + ".herokuapp.com"
    if os.getenv("CI")
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
    return requests.post((SERVER_URL + "/account/lookup"), json={"username": username})


def test_lookup(input_token):
    resp = lookup("testlookup@example.com")
    assert not resp.json()["exists"]

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


def checkVerified(username):
    return requests.post(
        (SERVER_URL + "/account/checkVerified"), json={"username": username}
    )


def makeVerified(username, token, input_token):
    return requests.post(
        (SERVER_URL + "/account/verifyUser"),
        json={"username": username, "token": token},
        headers={"Authorization": "Bearer " + input_token},
    )


def test_checkVerified(input_token):
    username = "testCheckVerified@example.com"
    resp = register(
        username, "password", "Test CheckVerified", "Here is some feedback.",
    )
    token = resp.json()["token"]
    resp = checkVerified(username)
    assert not resp.json()["verified"]
    resp = makeVerified(username, token, input_token)
    assert resp.json()["verified"]
    resp = checkVerified(username)
    assert resp.json()["verified"]
    delete(username, input_token)


def reset(username, new_password):
    return requests.post(
        (SERVER_URL + "/account/reset"),
        json={"username": username, "password": new_password},
    )


def test_reset(input_token):
    username = "testReset@example.com"
    register(
        username, "password", "Test Reset", "Here is some feedback.",
    )
    new_password = "new_password123"
    resp = login(username, new_password)
    assert not resp.json()["verified"]
    resp = reset(username, new_password)
    assert resp.json()["status"] == 200
    resp = login(username, new_password)
    assert resp.json()["verified"]
    delete(username, input_token)


def createFromImage(username):
    return requests.post(
        (SERVER_URL + "/disk/createFromImage"),
        json={
            "username": username,
            "location": "southcentralus",
            "vm_size": 10,
            "apps": [],
        },
    )


def getStatus(id):
    resp = requests.get((SERVER_URL + "/status/" + id))
    return resp.json()


def fetchDisks(username):
    return requests.post(
        (SERVER_URL + "/user/fetchdisks"), json={"username": username}
    ).json()


def test_fetchDisks(input_token):
    username = "testDisks@example.com"
    register(
        username, "password", "Test fetchDisks", "Some more feedback.",
    )

    resp = fetchDisks(username)
    assert len(resp["disks"]) == 0

    resp = createFromImage(username)
    assert resp.status_code == 202
    id = resp.json()["ID"]
    status = "PENDING"
    while status == "PENDING" or status == "STARTED":
        time.sleep(5)
        status = getStatus(id)["state"]
    assert status == "SUCCESS"
    disk_name = getStatus(id)["output"]

    resp = fetchDisks(username)
    assert len(resp["disks"]) > 0
    assert disk_name in map(lambda d: d["disk_name"], resp["disks"])

    delete(username, input_token)
