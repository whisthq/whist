import sys
import os
import pytest
import requests
from dotenv import load_dotenv
import time

load_dotenv()
SERVER_URL = (
    "https://main-webserver-pr-70" + ".herokuapp.com"
    if os.getenv("HEROKU_APP_NAME")
    else "http://localhost:5000"
)


def getStatus(id):
    resp = requests.get((SERVER_URL + "/status/" + id))
    return resp.json()


def createEmpty(disk_size, username):
    return requests.post(
        (SERVER_URL + "/disk/createEmpty"),
        json={"disk_size": disk_size, "username": username},
    )


def test_createEmpty(input_token):
    resp = createEmpty(10, "doesntexist@fake.com")
    assert resp.status_code == 404
    # Create an account and test it
    requests.post(
        (SERVER_URL + "/account/register"),
        json={
            "username": "fakefake@delete.com",
            "password": "password",
            "name": "fake",
            "feedback": "Where are average things manufactured? The satisfactory.",
        },
    )
    resp = createEmpty(10, "fakefake@delete.com")
    assert resp.status_code == 202
    id = resp.json()["ID"]
    status = "PENDING"
    while status == "PENDING" or status == "STARTED":
        time.sleep(5)
        status = getStatus(id)["state"]
    assert status == "SUCCESS"
    disk_name = getStatus(id)["output"]

    resp = requests.post(
        (SERVER_URL + "/disk/fetchDisk"),
        json={"disk_name": disk_name},
        headers={"Authorization": "Bearer " + input_token},
    )
    assert resp.json()["disk"] is not None

    requests.post(
        (SERVER_URL + "/disk/delete"),
        json={"username": "fakefake@delete.com"},
        headers={"Authorization": "Bearer " + input_token},
    )

    requests.post(
        (SERVER_URL + "/account/delete"),
        json={"username": "fakefake@delete.com"},
        headers={"Authorization": "Bearer " + input_token},
    )
