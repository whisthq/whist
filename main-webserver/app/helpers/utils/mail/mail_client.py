"""The Fractal mail client.

The MailClient class wraps the Python Sendgrid SDK to make it easy to send mail to users.

Email templates are stored in the `sales.email_templates table of the database.
MailClient.send_email looks up a template by its template ID, downloads it from the
fractal-email-templates S3 bucket, renders it with the specified parameters, and sends it to the
specified recipient.

Example:
    Suppose you've added an email template whose template ID is "HELLO" to the database. Let's
    pretend it's a Jinja template string that can be rendered with a particular message.

    The following lines of code can be used to render your template and send it to
    recipient@example.com with the message "Hello".

        mc = MailClient("...")
        mc.send_email("HELLO", "recipient@example.com", jinja_args={"message": "Hello"})

"""

import re
from jinja2 import Template
from python_http_client.exceptions import BadRequestsError
from sendgrid import SendGridAPIClient
from sendgrid.helpers.mail import Mail

from app.helpers.utils.general.logs import fractal_logger
from app.exceptions import SendGridException, TemplateNotFound
from app.models import EmailTemplates
from app.helpers.utils.general.tokens import get_access_tokens


class MailClient:
    """A helper class that uses the Sendgrid API to send emails to users."""

    def __init__(self, api_key):
        """Initialize a reusable MailClient object.

        Args:
            api_key (str): SendGrid private API key
        """
        self.sendgrid_client = SendGridAPIClient(api_key)

    def send_email(
        self,
        email_id,
        to_email,
        from_email="noreply@fractal.co",
        jinja_args=None,
    ):
        """Sends a rendered email template.

        Args:
            email_id (str): The database ID (i.e. the value of the sales.email_templates.id column)
                of the email template to render.
            to_email (str): The recipient's email address as a string.
            from_email (str): Optional. The sender's email address as a string. Defaults to
                "noreply@fractal.co".
            jinja_args (dict): Optional. Keyword arguments forwarded to Jinja's Template.render().
                Defaults to {}.

        Returns:
            None

        Raises:
            SendGridException: Sendgrid was not able to send an email. SendGridAPIClient.send()
                has a tendency to raise cryptic errors. Any time SendGridException is raised, the
                traceback will also include the traceback of the SendGridAPIClient.send() error
                that directly triggered SendGridException.
            TemplateNotFound: There does not exist a record in the database of an email template
                whose ID matches email_id.
        """

        template = EmailTemplates.query.get(email_id)

        if template is None:
            raise TemplateNotFound(email_id)

        fractal_logger.info(f"Downloading {template.url} from Amazon S3...")

        template_string = template.download()

        fractal_logger.info("Checking arguments...")

        if not MailUtils.check_jinja_args(template_string, jinja_args):
            raise SendGridException

        fractal_logger.info("Sanitizing arguments...")

        sanitized_args = MailUtils.sanitize_jinja_args(
            to_email, jinja_args if jinja_args is not None else {}
        )

        fractal_logger.info(f"Rendering {template.id} with {sanitized_args}...")

        jinja_template = Template(template_string)
        message = Mail(
            from_email=from_email,
            to_emails=to_email,
            subject=template.title,
            html_content=jinja_template.render(**sanitized_args),
        )

        fractal_logger.info(f"Sending '{template.title}' from {from_email} to {to_email}...")

        try:
            self.sendgrid_client.send(message)
        except BadRequestsError as e:
            # An invalid to_emails argument may have been passed to the Mail class constructor.
            raise SendGridException from e


class MailUtils:
    """
    Contains all static methods for MailClient (above)
    """

    @staticmethod
    def check_jinja_args(template, jinja_args):
        """Checks that jinja_args contains all the template arguments

        Args:
            template (str): Email template from S3
            jinja_args (dict): Dict of arguments to pass to Jinja

        Returns:
            True if jinja_args contains all args, False otherwise

        """
        template_args = re.findall("{{([^}]+)}}", template)
        jinja_keys = jinja_args.keys()

        for template_arg in template_args:
            if template_arg not in jinja_keys:
                return False
        return True

    @staticmethod
    def sanitize_jinja_args(to_email, jinja_args):
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
                    access_token, _ = get_access_tokens(to_email)
                    jinja_args["link"] = "{base_url}{email_verification_token}".format(
                        base_url=jinja_args["link"].split("reset?")[0] + "reset?",
                        email_verification_token=access_token,
                    )

        return jinja_args
