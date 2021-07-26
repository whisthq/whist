from app import ma
from app.models import EmailTemplates


class EmailTemplatesSchema(ma.SQLAlchemyAutoSchema):
    """Serialize an instance of the EmailTemplate model into a host-service-readable format."""

    class Meta:
        model = EmailTemplates
