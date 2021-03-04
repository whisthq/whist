import logging
import requests
from jinja2 import Template

from sendgrid import SendGridAPIClient
from sendgrid.helpers.mail import Mail
from app.helpers.utils.general.logs import fractal_log
from app.models import User, EmailTemplates
from app.exceptions import TemplateNotFound, SendGridException

# Welcome to the mail client. The mail client uses the Sendgrid API to
# send emails to users.

# Let's say you want to send an email to a user. Here's how to do it:

# 1. Create an email template as a .html file and upload it to the fractal-email-templates
#    bucket in S3. You can use Jinja formatting for variables. Make sure it's set to public.
# 2. Add the email template to the sales.email_templates table in the SQL database.
# 3. get_available_templates() will pull all the email templates from the
#    SQL table, and MailClient.send_email will send any email template.


class MailClient:
    """A helper class that uses the Sendgrid API to send emails to users.

    To use the MailClient class, follow these steps:
    1. Create an email template as a .html file and upload it to the fractal-email-templates
        bucket in S3. You can use Jinja formatting for variables. Make sure it's set to public.
    2. Add the email template to the sales.email_templates table in the SQL database.
    3. get_available_templates() will pull all the email templates from the
        SQL table, and MailClient.send_email will send any email template.

    Attributes:
        api_key (str): Sendgrid api key that the MailClient uses to create the SendGridAPIClient
    """

    def __init__(self, api_key):
        """Initialize a reusable MailClient object.

        Args:
            api_key (str): SendGrid private API key
        """
        self.sendgrid_client = SendGridAPIClient(api_key)

    @staticmethod
    def sanitize_jinja_args(self, to_email, jinja_args):
        """Do custom server-side replacement of certain Jinja args,
        for example retrieving email verification tokens from the database

        Args:
            to_email (str): Email address to send to
            jinja_args (dict): Dict of arguments to pass to Jinja
        """

        if jinja_args:
            jinja_keys = jinja_args.keys()
            if "link" in jinja_keys:
                if "reset?" in jinja_args["link"]:
                    user = User.query.get(to_email)
                    if user:
                        jinja_args["link"] = "{base_url}{email_verification_token}".format(
                            base_url=jinja_args["link"].split("reset?")[0],
                            email_verification_token=user.token,
                        )

        return jinja_args

    def send_email(
        self,
        to_email,
        from_email="noreply@fractal.co",
        subject="",
        html_file=None,
        email_id=None,
        jinja_args=None,
    ):
        """Sends an email via Sendgrid with a given subject and HTML file (for email body)

        Args:
            from_email (str): Email address where the email is coming from
            to_email (str): Email address to send to
            subject (str): Email title
            html_file (str): file of HTML content e.g. example.html
            email_id (str): Email ID that maps to html_file, found in database.
                NOTE: Either email_id or html_file must be provided. If both are provided,
                    email_id is used.
            jinja_args (dict): Dict of Jinja arguments to pass into render_template()

        Raises:
            NonexistentEmail: If any of from_email or to_emails does not exist
        """
        if not html_file and not email_id:
            raise TemplateNotFound(email_id)

        if email_id:
            templates = get_available_templates()

            if not email_id in templates.keys():
                raise TemplateNotFound(email_id)

            html_file_url = str(templates[email_id]["url"])
            subject = str(templates[email_id]["title"])

            fractal_log(
                logs=f"Sending {html_file_url} with subject {subject}",
                label=from_email,
                function="send_email",
            )
            try:
                html_as_string = requests.get(html_file_url)
            except Exception as e:
                fractal_log(
                    logs=f"An exception occured: {str(e)}",
                    label=from_email,
                    function="send_email",
                    level=logging.ERROR,
                )
                raise Exception from e
            jinja_template = Template(html_as_string.text)

        elif html_file:
            try:
                jinja_template = Template(open(html_file).read())
            except Exception as e:
                fractal_log(
                    logs=f"An exception occured: {str(e)}",
                    label=from_email,
                    function="send_email",
                    level=logging.ERROR,
                )
                raise IOError from e

        try:
            jinja_args = self.sanitize_jinja_args(to_email, jinja_args)
            message = Mail(
                from_email=from_email,
                to_emails=to_email,
                subject=subject,
                html_content=jinja_template.render(**jinja_args),
            )
            self.sendgrid_client.send(message)
        except Exception as e:
            fractal_log(
                logs=f"An exception occured: {str(e)}",
                label=from_email,
                function="send_email",
                level=logging.ERROR,
            )
            raise SendGridException from e


def get_available_templates():
    """Retrieves all available HTML email templates stored in S3

    Args:
        none

    Returns:
        dict: Dictionary mapping email template ID (string) to HTML url (string)
    """

    templates = EmailTemplates.query.all()
    return {template.id: {"url": template.url, "title": template.title} for template in templates}
