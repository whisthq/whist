from tests import *


@pytest.fixture
def input_token():
    resp = requests.post(
        (SERVER_URL + "/account/login"),
        json=dict(
            username="fractal-admin@gmail.com", password="!!fractal-admin-password!!"
        ),
    )

    input_token = resp.json()["access_token"]

    return input_token
