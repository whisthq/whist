"""Serializers for models that model data in the oauth schema."""

from app import ma
from app.models import Credential


class CredentialSchema(ma.SQLAlchemyAutoSchema):
    class Meta:
        model = Credential
