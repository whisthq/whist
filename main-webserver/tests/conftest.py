import os

from tests import *
from dotenv import *

load_dotenv(find_dotenv())

@pytest.fixture
def input_token():
    resp = requests.post(
        (SERVER_URL + "/account/login"),
        json=dict(
            username="fractal-admin@gmail.com", password="!!fractal-admin-password!!"
        ),
    )

    return resp.json()["access_token"]

@pytest.fixture
def admin_token():
    resp = requests.post(
        (SERVER_URL + "/admin/login"),
        json=dict(
            username=os.getenv("DASHBOARD_USERNAME"), password=os.getenv("DASHBOARD_PASSWORD")
        ),
    )

    return resp.json()["access_token"]
