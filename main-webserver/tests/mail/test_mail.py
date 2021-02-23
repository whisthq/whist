"""Tests for the /mail endpoint."""

import pytest
import mailslurp_client
from http import HTTPStatus
from flask import current_app

from app.helpers.utils.mail.mail_client import MailClient, TemplateNotFound, SendGridException


@pytest.fixture(scope="session")
def get_mailslurp_key():
    """Gets the mail client api key
    """
    return "ade68d47a8ba39c57a8e8358e5e86d6d99a04cf8aeebf9c11c08f851f2fa438f"

@pytest.fixture(scope="session")
def mail_client(app):
    """Makes a mail client with an api key (setting the global api key).

    Returns:
        (str): client with api key initialized.
    """
    return MailClient(app.config["SENDGRID_API_KEY"])


@pytest.fixture(scope="session")
def mailslurp(get_mailslurp_key):
    configuration = mailslurp_client.Configuration()
    configuration.api_key[
        "x-api-key"
    ] = get_mailslurp_key

    api_client = mailslurp_client.ApiClient(configuration)

    return api_client


@pytest.fixture
def email():
    """Makes an example email

    Returns:
        (str): Sender email
    """

    def _email(from_email, subject, email_id):
        return {"from_email": from_email, "subject": subject, "email_id": email_id}

    return _email


@pytest.mark.parametrize(
    "from_email, subject, email_id, expected",
    [
        (
            "noreply@fractal.co",
            "Thank you for choosing Fractal",
            "PAYMENT_SUCCESSFUL",
            HTTPStatus.OK,
        ),
        ("", "Thank you for choosing Fractal", "PAYMENT_SUCCESSFUL", HTTPStatus.BAD_REQUEST),
        ("noreply@fractal.co", "Thank you for choosing Fractal", "", HTTPStatus.BAD_REQUEST),
    ],
)
def test_send_emails(mailslurp, email, client, from_email, subject, email_id, expected):
    """Tests sample emails to a valid email address"""
    api_instance = mailslurp_client.InboxControllerApi(mailslurp)
    inbox = api_instance.create_inbox()

    test_email = email(from_email, subject, email_id)

    response = client.post(
        "/mail",
        json=dict(
            email_id=test_email["email_id"],
            from_email=test_email["from_email"],
            to_email=inbox.email_address,
            email_args={},
        ),
    )
    assert response.status_code == expected

    if response.status_code == HTTPStatus.OK:
        waitfor_controller = mailslurp_client.WaitForControllerApi(mailslurp)
        email = waitfor_controller.wait_for_latest_email(
            inbox_id=inbox.id, timeout=30000, unread_only=True
        )

        assert email.subject == test_email["subject"]


def test_send_to_bad_email(client):
    """Tests sending a sample email to an invalid email address"""
    response = client.post(
        "/mail",
        json=dict(
            email_id="EMAIL_VERIFICATION",
            from_email="noreply@fractal.co",
            to_email="",
            email_args={},
        ),
    )
    assert response.status_code == HTTPStatus.BAD_REQUEST
