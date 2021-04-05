"""Tests for the /mail endpoint."""

from http import HTTPStatus

import pytest
import mailslurp_client

from app.helpers.utils.mail.mail_client import MailClient
from tests.constants.api_keys import MAILSLURP_API_KEY


@pytest.fixture(scope="session")
def get_mailslurp_key():
    """Gets the mail client api key"""
    return MAILSLURP_API_KEY


@pytest.fixture(scope="session")
def mail_client(app):
    """Makes a mail client with an api key (setting the global api key).

    Returns:
        (str): client with api key initialized.
    """
    return MailClient(app.config["SENDGRID_API_KEY"])


@pytest.fixture(scope="session")
def mailslurp(get_mailslurp_key):
    """Create a MailSlurp client with the MailSlurp api key

    Returns:
        A MailSlurp client with which to mock email sending
    """
    configuration = mailslurp_client.Configuration()
    configuration.api_key["x-api-key"] = get_mailslurp_key

    api_client = mailslurp_client.ApiClient(configuration)

    return api_client


@pytest.fixture(scope="session")
def email():
    """Generates an inner function that takes in email details and formats them into a dictionary."""

    def _email(from_email, subject, email_id):
        """Creates a dictionary with the given email details

        Args:
            from_email (str): email from which the email should be sent
            subject (str): subject line of the email
            email_id (str): id of the email template being sent out
        """
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
@pytest.mark.usefixtures("authorized")
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


@pytest.mark.usefixtures("authorized")
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


@pytest.mark.usefixtures("authorized")
def test_send_template_bad_args(client):
    """Tsets sending a sample email with bad jinja_args"""
    response = client.post(
        "/mail",
        json=dict(
            email_id="EMAIL_VERIFICATION",
            from_email="noreply@fractal.co",
            to_email="",
            email_args={"bad_arg": "bad_arg"},
        ),
    )
    assert response.status_code == HTTPStatus.BAD_REQUEST
