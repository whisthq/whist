from app import ma
from app.models import Credential


class CredentialSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = Credential
