import pytest

from app.constants.http_codes import UNAUTHORIZED
from app.helpers.utils.general.logs import fractal_logger


@pytest.fixture
def bulk_register_user(client):
    """
    Use this fixture to create users via the /account endpoint. Note that
    these are rate-limited, so you can only create a select number per test
    (or change this function to reset the rate limiter). This fixture makes
    sure the user is created properly and handles cleanup of the users after
    the test finishes.

    Usage:
    def my_test(bulk_register_user):
        # pass the paramaters to register_user here
        bulk_register_user(...)
    """
    # run before the test starts
    users = []
    access_tokens = []

    def register_user(username, password):
        users.append(username)
        resp = client.post(
            "/account/register",
            json={
                "username": username,
                "password": password,
                "name": "",
                "feedback": "",
                "encrypted_config_token": "",
            },
        )
        assert (
            resp.status_code == 200
        ), f"Error registering - got code {resp.status_code} and content {resp.get_data()}"
        # store access token so we can delete later
        access_token = resp.json["access_token"]
        access_tokens.append(access_token)
        return resp

    yield register_user

    # run after the test finishes
    for user, token in zip(users, access_tokens):
        resp = client.post(
            "/account/delete",
            json={
                "username": user,
            },
            headers={
                "Authorization": f"Bearer {token}",
            },
        )
        assert (
            resp.status_code == 200
        ), f"Error deleting - got code {resp.status_code} and content {resp.get_data()}"


def test_account_admin_security_flaw_fix(client, bulk_register_user):
    """
    This fixes a security flaw where someone could register with a @fractal.co username
    but never be verified. This gave them admin access to our webserver.
    """
    resp = bulk_register_user("super_duper_fake@fractal.co", "")
    access_token = resp.json["access_token"]
    # /dummy is a developer/admin protected endpoint
    resp = client.get(
        "/dummy",
        headers={
            "Authorization": f"Bearer {access_token}",
        },
    )
    assert resp.status_code == UNAUTHORIZED
