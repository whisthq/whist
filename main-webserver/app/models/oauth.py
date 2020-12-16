"""Data models for securely Fractal's third-party app credentials."""

import logging

from datetime import datetime, timezone

import requests

from flask import current_app
from google_auth_oauthlib.flow import Flow
from oauthlib.oauth2 import InvalidGrantError
from sqlalchemy_utils.types.encrypted.encrypted_type import AesEngine, StringEncryptedType

from app.helpers.utils.general.logs import fractal_log

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

    def refresh(self, cleanup=True, force=False):
        """Use this credential's refresh token to refresh the access token.

        This method modifies the instance on which it is called in place.

        Arguments:
            cleanup: A boolean indicating whether or not to delete a credential that cannot be
                refreshed from the database.
            force: A boolean indiciating whether or not to refresh the access token even if its
                expiration date has not yet passed.

        Raises:
            InvalidGrantError: The credential cannot be refreshed because the refresh token has
                been revoked or has expired.
        """

        if force or self.expiry <= datetime.now(timezone.utc):
            flow = Flow.from_client_config(current_app.config["GOOGLE_CLIENT_SECRET_OBJECT"], None)

            try:
                # The Flow class doesn't expose a refresh_token method directly, so we have to
                # access the underlying OAuth2Session instance.
                flow.oauth2session.refresh_token(
                    flow.client_config["token_uri"],
                    client_id=flow.client_config["client_id"],
                    client_secret=flow.client_config["client_secret"],
                    refresh_token=self.refresh_token,
                )
            except InvalidGrantError:
                if cleanup:
                    db.session.delete(self)
                    db.session.commit()

                raise

            self.access_token = flow.credentials.token
            self.expiry = flow.credentials.expiry
            assert flow.credentials.refresh_token == self.refresh_token

            db.session.add(self)
            db.session.commit()

    def revoke(self, cleanup=True):
        """Revoke the credential against the appropriate OAuth provider's servers.

        Arguments:
            cleanup: A boolean indicating whether or not to delete the credential from the
                database.
        """

        response = requests.post(
            "https://oauth2.googleapis.com/revoke",
            params={"token": self.refresh_token},
            headers={"Content-Type": "application/x-www-form-urlencoded"},
        )

        if response.ok:
            log_kwargs = {
                "logs": f"Successfully revoked external API credential for user '{self.user_id}'.",
                "level": logging.INFO,
            }
        else:
            log_kwargs = {
                "logs": (
                    f"Encountered an error while attempting to revoke external API credential for "
                    f"user '{self.user_id}': {response.text}"
                ),
                "level": logging.WARNING,
            }

        fractal_log(
            function="Credential.revoke",
            label=self.user_id,
            **log_kwargs,
        )

        if cleanup:
            db.session.delete(self)
            db.session.commit()
