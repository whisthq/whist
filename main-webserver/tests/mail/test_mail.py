import mailslurp_client

from helpers.utils.mail.mail_client import MailClient, TemplateNotFound, SendGridException


@pytest.fixture(scope="session")
def mail_client(app):
    """Makes a mail client with an api key (setting the global api key).

    Returns:
        (str): client with api key initialized.
    """
    return MailClient(app.config["STRIPE_SECRET"])

@pytest.fixture(scope="session")
def mailslurp():
    configuration = mailslurp_client.Configuration()
    configuration.api_key['x-api-key'] = "ade68d47a8ba39c57a8e8358e5e86d6d99a04cf8aeebf9c11c08f851f2fa438f"

    api_client = mailslurp_client.ApiClient(configuration)

    return api_client

@pytest.fixture(scope="session")
def example_email():
    """Makes an example email

    Returns:
        (str): Sender email
    """
    return {
        "from_email": "noreply@fractal.co",
        "subject": "Example Subject",
        "html_file": "https://fractal-email-templates.s3.amazonaws.com/payment_successful.html",
        "email_id": "PAYMENT_SUCCESSFUL"
    }


def test_send_email(mailslurp, example_email):
    api_instance = mailslurp_client.InboxControllerApi(mailslurp)
    inbox = api_instance.create_inbox()

    output = mail_client.send_email(
        to_email=inbox.email_address,
        from_email=example_email["from_email"],
        subject=example_email["subject"],
        html_file=example_email["html_file"],
        jinja_args={}
    )

    waitfor_controller = mailslurp_client.WaitForControllerApi(mailslurp)
    email = waitfor_controller.wait_for_latest_email(inbox_id=inbox.id, timeout=30000, unread_only=True)

    assert email.subject == example_email["subject"]
