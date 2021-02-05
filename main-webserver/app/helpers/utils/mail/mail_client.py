import logging

from flask import current_app, render_template
from sendgrid import SendGridAPIClient
from sendgrid.helpers.mail import Mail
from app.helpers.utils.general.logs import fractal_log
from app.models import EmailTemplates

""" Welcome to the mail client. The mail client uses the Sendgrid API to 
send emails to users.
"""

class TemplateNotFound(Exception):
    """This exception is raised when an email template ID is requested that does not exist"""

    pass

class SendGridException(Exception):
    """This exception is raised when SendGrid email sending API throws an exception"""

    pass


class MailClient:
    # api key must be the stripe secret key or
    # a restrited "secret" key as in the stripe dashboard
    # go to the keys section on the bottom left and search for "limitted access"
    def __init__(self, api_key):
        """Initialize a reusable MailClient object.

        Args:
            api_key (str): SendGrid private API key
        """
        self.sendgrid_client = SendGridAPIClient(api_key)

    def send_email(self, from_email, to_emails, subject, html_file, jinja_args = None):
        """Sends an email via Sendgrid with a given subject and HTML file (for email body)

        Args:
            from_email (str): Email address where the email is coming from
            to_emails (list): List of email addresses to send to
            subject (str): Email title
            html_file (str): File of HTML content e.g. example.html
            jinja_args (dict): Dict of Jinja arguments to pass into render_template()

        Raises:
            NonexistentEmail: If any of from_email or to_emails does not exist
        """

        try:
            message = Mail(
                from_email=from_email,
                to_emails=to_emails,
                subject=subject,
                html_content=render_template(html_file, **jinja_args),
            )
            self.sendgrid_client.send(message)
        except Exception as e:
            raise SendGridException

    def get_available_templates(self):
        """Retrieves all available HTML email templates stored in S3

        Args:
            none

        Returns:
            dict: Dictionary mapping email template ID (string) to HTML url (string)
        """

        templates = EmailTemplates.query.all()
        return {template.id: {url: template.url, title: template.title} for template in templates}
