from ._meta import db


class EmailTemplates(db.Model):
    __tablename__ = "email_templates"
    __table_args__ = {"schema": "sales", "extend_existing": True}

    id = db.Column(db.String(250), primary_key=True)
    url = db.Column(db.String(250), nullable=False)
