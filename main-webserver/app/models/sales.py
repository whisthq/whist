from ._meta import db


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
