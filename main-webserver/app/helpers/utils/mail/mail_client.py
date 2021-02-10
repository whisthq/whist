import logging
import json
import requests
from bs4 import BeautifulSoup
from jinja2 import Template

from flask import current_app, redirect
from sendgrid import SendGridAPIClient
from sendgrid.helpers.mail import Mail
from app.helpers.utils.general.logs import fractal_log
from app.models import EmailTemplates

""" 
Welcome to the mail client. The mail client uses the Sendgrid API to 
send emails to users.

Let's say you want to send an email to a user. Here's how to do it:

1. Create an email template as a .html file and upload it to the fractal-email-templates
   bucket in S3. You can use Jinja formatting for variables. Make sure it's set to public.
2. Add the email template to the sales.email_templates table in the SQL database.
3. MailClient.get_available_templates() will pull all the email templates from the 
   SQL table, and MailClient.send_email will send any email template.
"""


class TemplateNotFound(Exception):
    """This exception is raised when an email template ID is requested that does not exist"""

    pass


class SendGridException(Exception):
    """This exception is raised when SendGrid email sending API throws an exception"""

    pass


class MailClient:
    def __init__(self, api_key):
        """Initialize a reusable MailClient object.

        Args:
            api_key (str): SendGrid private API key
        """
        self.sendgrid_client = SendGridAPIClient(api_key)

    def send_email(
        self,
        to_email,
        from_email="support@fractal.co",
        subject="",
        html_file=None,
        email_id=None,
        jinja_args={},
    ):
        """Sends an email via Sendgrid with a given subject and HTML file (for email body)

        Args:
            from_email (str): Email address where the email is coming from
            to_emails (list): List of email addresses to send to
            subject (str): Email title
            html_file (str): File of HTML content e.g. example.html
            email_id (str): Email ID that maps to html_file, found in database. 
                NOTE: Either email_id or html_file
                must be provided. If both are provided, email_id is used.
            jinja_args (dict): Dict of Jinja arguments to pass into render_template()

        Raises:
            NonexistentEmail: If any of from_email or to_emails does not exist
        """
        if not html_file and not email_id:
            raise TemplateNotFound

        if email_id:
            templates = self.get_available_templates()

            if not email_id in templates.keys():
                raise TemplateNotFound

            html_file = str(templates[email_id]["url"])
            subject = str(templates[email_id]["title"])

        try:
            fractal_log(
                logs=f"Sending {html_file} with subject {subject}",
                label=from_email,
                function="send_email",
            )

            html_as_string = requests.get(html_file)
            jinja_template = Template(html_as_string.text)

            message = Mail(
                from_email=from_email,
                to_emails=to_email,
                subject=subject,
                html_content=jinja_template.render(**jinja_args),
            )
            response = self.sendgrid_client.send(message)
        except Exception as e:
            fractal_log(
                logs=f"An exception occured: {str(e)}", label=from_email, function="send_email", level=logging.ERROR
            )
            raise SendGridException

    def get_available_templates(self):
        """Retrieves all available HTML email templates stored in S3

        Args:
            none

        Returns:
            dict: Dictionary mapping email template ID (string) to HTML url (string)
        """

        templates = EmailTemplates.query.all()
        return {
            template.id: {"url": template.url, "title": template.title} for template in templates
        }
