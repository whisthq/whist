import sys
import os
import pytest
import requests
from dotenv import load_dotenv
import time

load_dotenv()
SERVER_URL = (
    "https://main-webserver-pr-" + str(os.getenv("TEST_HEROKU_PR_NUMBER")) + ".herokuapp.com"
    if os.getenv("CI") == "true"
    else "http://localhost:5000"
)


def getStatus(id):
    resp = requests.get((SERVER_URL + "/status/" + id))
    return resp.json()


def createEmpty(disk_size, username, input_token):
    return requests.post(
        (SERVER_URL + "/disk/createEmpty"),
        json={"disk_size": disk_size, "username": username},
        headers={"Authorization": "Bearer " + input_token},
    )


def test_createEmpty(input_token):
    resp = createEmpty(10, "doesntexist@fake.com", input_token)
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
    resp = createEmpty(10, "fakefake@delete.com", input_token)
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
