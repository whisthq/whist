from ._meta import db


class EmailTemplates(db.Model):
    """Stores all email templates that could be sent out to users. Use for the MailClient
    in app/helpers/utils/mail.py.

    Args:
        db (SQLAlchemy db): Implements db methods to communicate with the physical infra.
    """

    __tablename__ = "email_templates"
    __table_args__ = {"schema": "sales", "extend_existing": True}

    id = db.Column(db.String(250), primary_key=True)
    url = db.Column(db.String(250), nullable=False)
    title = db.Column(db.String(250), nullable=False)
