import pytest
import requests
from dotenv import load_dotenv

pytest_plugins = ["helpers_namespace"]

load_dotenv()
SERVER_URL = (
    "https://main-webserver-pr-"
    + str(os.getenv("TEST_HEROKU_PR_NUMBER"))
    + ".herokuapp.com"
    if os.getenv("CI") == "true"
    else "http://localhost:5000"
)


@pytest.fixture
def input_token():
    print(SERVER_URL)
    resp = requests.post(
        (SERVER_URL + "/account/login"),
        json=dict(username="whatever", password="whatever"),
    )
    return resp.json()["access_token"]


@pytest.helpers.register
def login(username, password):
    return requests.post(
        (SERVER_URL + "/account/login"), json=dict(username=username, password=password)
    )


@pytest.helpers.register
def register_user(username, password, name, feedback):
    return requests.post(
        (SERVER_URL + "/account/register"),
        json={
            "username": username,
            "password": password,
            "name": name,
            "feedback": feedback,
        },
    )


@pytest.helpers.register
def deleteUser(username, authToken):
    return requests.post(
        (SERVER_URL + "/account/delete"),
        json={"username": username},
        headers={"Authorization": "Bearer " + authToken},
    )


@pytest.helpers.register
def adminLogin(username, password):
    return requests.post(
        (SERVER_URL + "/admin/login"), json=dict(username=username, password=password)
    )


@pytest.helpers.register
def lookup(username):
    return requests.post((SERVER_URL + "/account/lookup"), json={"username": username})


@pytest.helpers.register
def checkVerified(username):
    return requests.post(
        (SERVER_URL + "/account/checkVerified"), json={"username": username}
    )


@pytest.helpers.register
def makeVerified(username, token, input_token):
    return requests.post(
        (SERVER_URL + "/account/verifyUser"),
        json={"username": username, "token": token},
        headers={"Authorization": "Bearer " + input_token},
    )


@pytest.helpers.register
def reset(username, new_password):
    return requests.post(
        (SERVER_URL + "/account/reset"),
        json={"username": username, "password": new_password},
    )


@pytest.helpers.register
def createFromImage(username, input_token):
    return requests.post(
        (SERVER_URL + "/disk/createFromImage"),
        json={
            "username": username,
            "location": "southcentralus",
            "vm_size": 10,
            "apps": [],
        },
        headers={"Authorization": "Bearer " + input_token},
    )


@pytest.helpers.register
def getStatus(id):
    resp = requests.get((SERVER_URL + "/status/" + id))
    return resp.json()


@pytest.helpers.register
def fetchDisks(username):
    return requests.post(
        (SERVER_URL + "/user/fetchdisks"), json={"username": username}
    ).json()


@pytest.helpers.register
def deleteDisks(username, input_token):
    return requests.post(
        (SERVER_URL + "/disk/delete"),
        json={"username": username},
        headers={"Authorization": "Bearer " + input_token},
    )
