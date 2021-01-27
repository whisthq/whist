"""Serializers for models that model data in the oauth schema."""

from app import ma
from app.models import Credential
from app.models.oauth import _provider_id_to_app_name


class CredentialSchema(ma.SQLAlchemySchema):
    """Serialize an instance of the Credntial model into a host-service-readable format.

    This serializer serializes a subset of the model's attributes. The provider_id attribute on
    the model instance is mapped to an external application code name and renamed to "provider" in
    the output dictionary.
    """

    class Meta:
        model = Credential

    access_token = ma.auto_field()
    provider = ma.Function(lambda credential: _provider_id_to_app_name(credential.provider_id))
    expiry = ma.auto_field()
    refresh_token = ma.auto_field()
    token_type = ma.auto_field()
