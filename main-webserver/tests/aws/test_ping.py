"""Tests for the /container/ping endpoint."""

import pytest

from app import db
from app.celery.aws_ecs_status import pingHelper
from app.constants.http_codes import SUCCESS
from app.helpers.utils.stripe.stripe_payments import stripeChargeHourly


def status(code):
    return eval(f"""lambda *args, **kwargs: {{"status": {code}}}""")


@pytest.fixture
def no_stripe(monkeypatch):
    monkeypatch.setattr(
        stripeChargeHourly,
        "__code__",
        (lambda _: None).__code__,
    )


def test_no_availability(client):
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_ping(omit_available=True)

    assert response.status_code == 400


def test_no_port(client):
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_ping(omit_port=True)

    assert response.status_code == 400


def test_not_found(client, monkeypatch):
    code = 404

    monkeypatch.setattr(pingHelper, "apply_async", status(code))
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_ping(available=True, port=0)

    assert response.status_code == code


def test_successful(client, monkeypatch):
    code = 200

    monkeypatch.setattr(pingHelper, "apply_async", status(code))
    client.login("new-email@fractalcomputers.com", "new-email-password")

    response = client.container_ping(available=True, port=0)

    assert response.status_code == code


def test_no_container(container, no_stripe):
    with container("RUNNING_AVAILABLE"):
        with pytest.raises(Exception):
            pingHelper(True, "x.x.x.x", 0)


@pytest.mark.parametrize(
    "available, initial_state, final_state",
    (
        (True, "CREATING", "RUNNING_AVAILABLE"),
        (True, "RUNNING_AVAILABLE", "RUNNING_AVAILABLE"),
        (True, "RUNNING_UNAVAILABLE", "RUNNING_AVAILABLE"),
        (True, "STOPPING", "STOPPING"),
        (False, "CREATING", "RUNNING_UNAVAILABLE"),
        (False, "RUNNING_AVAILABLE", "RUNNING_UNAVAILABLE"),
        (False, "RUNNING_UNAVAILABLE", "RUNNING_UNAVAILABLE"),
        (False, "STOPPING", "STOPPING"),
    ),
)
def test_ping_helper(available, container, final_state, initial_state, no_stripe):
    with container(initial_state) as c:
        result = pingHelper(available, c.ip, c.port_32262)

        assert "status" in result
        assert result["status"] == SUCCESS

        # I don't totally understand why this line is necessary, but it seems
        # to be. -O
        db.session.add(c)

        assert c.state == final_state

        history = c.user.history

        if initial_state == "RUNNING_AVAILABLE" and final_state == "RUNNING_UNAVAILABLE":
            # Make sure the login was recorded in the database.
            login = history.first()

            assert login
            assert login.action == "logon"
        elif initial_state == "RUNNING_UNAVAILABLE" and final_state == "RUNNING_AVAILABLE":
            # Make sure the logout was recorded in the database.
            logout = history.first()

            assert logout
            assert logout.action == "logoff"
        else:
            # Make sure that neither any login nor logout events have been
            # recorded.
            assert not db.session.query(history.exists()).scalar()
