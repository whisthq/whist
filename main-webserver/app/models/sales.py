import os.path
import urllib.parse

import boto3

from ._meta import db


def _parse_object_key(url: str) -> str:
    """Extract the name of an S3 object from its HTTP or S3 URL.

    Example:

        >>> from app.models.sales import _parse_object_key
        >>> _parse_object_key("s3://fractal-email-templates/password_reset.html")
        'password_reset.html'
        >>> _parse_object_key(
        ...     "https://fractal-email-templates.s3.amazonaws.com/feedback.html"
        ... )
        'feedback.html'
        >>>

    Args:
        url: The object's HTTP or S3 URL.

    Returns:
        A string representing the name of the object in its S3 bucket.
    """

    return os.path.basename(urllib.parse.urlparse(url).path)


class EmailTemplates(db.Model):
    """Stores all email templates that could be sent out to users.

    Used in the MailClient in app/helpers/utils/mail.py.

    Attributes:
        id (str): string used to identify email templates and
                  differentiation from each other
        url (str): url to the website template living in the AWS S3 bucket
        title (str): subject of the email
    """

    __tablename__ = "email_templates"
    __table_args__ = {"schema": "sales", "extend_existing": True}

    id = db.Column(db.String(250), primary_key=True)
    url = db.Column(db.String(250), nullable=False)
    title = db.Column(db.String(250), nullable=False)

    def download(self) -> str:
        """Download the template from Amazon S3.

        Returns:
            The contents of the Jinja template file as a string.
        """

        object_key = _parse_object_key(self.url)
        s3_client = boto3.client("s3", region_name="us-east-1")
        response = s3_client.get_object(Bucket="fractal-email-templates", Key=object_key)

        return response["Body"].read().decode()  # Ftype: ignore[no-any-return]
