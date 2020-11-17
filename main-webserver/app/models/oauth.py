"""Data models for securely Fractal's third-party app credentials."""

from flask import current_app
from sqlalchemy_utils.types.encrypted.encrypted_type import AesEngine, StringEncryptedType

from ._meta import db


def secret_key():
    """Retrieve the secret key from the Flask application's configuration.

    Returns:
        The value of the SECRET_KEY configuration variable, a string.
    """

    return current_app.secret_key


class Credential(db.Model):
    """Store encrypted OAuth tokens in the database.

    Attributes:
        access_token: An OAuth access token.
        expiry: The date and time (with time zone) at which the access token expires.
        refresh_token: An OAuth refresh token.
        token_type: Always "bearer".
        user_id: The user_id of the user who owns the OAuth token.
    """

    __tablename__ = "credentials"
    __table_args__ = {"extend_existing": True, "schema": "oauth"}

    access_token = db.Column(
        StringEncryptedType(db.String, secret_key, AesEngine, "pkcs5"), nullable=False
    )
    expiry = db.Column(db.DateTime(timezone=True), nullable=False)
    refresh_token = db.Column(StringEncryptedType(db.String, secret_key, AesEngine, "pkcs5"))
    token_type = db.Column(db.String(128), nullable=False)
    user_id = db.Column(db.ForeignKey("users.user_id"), nullable=False, primary_key=True)
