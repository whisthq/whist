from app import ma
from app.models import EmailTemplates


class EmailTemplatesSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = EmailTemplates
