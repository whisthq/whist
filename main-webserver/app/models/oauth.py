"""Data models for securely Fractal's third-party app credentials."""

from datetime import datetime, timezone

import requests

from dropbox import Dropbox
from flask import current_app
from google_auth_oauthlib.flow import Flow
from oauthlib.oauth2 import InvalidGrantError
from sqlalchemy_utils.types.encrypted.encrypted_type import AesEngine, StringEncryptedType

from app.helpers.utils.general.logs import fractal_logger

from ._meta import db, secret_key


class OAuthAppError(Exception):
    """Indicate that the specified external application is not recognized.

    Raised when an unrecognized external application code name is specified during an operation.

    Arguments:
        app_name: The unrecognized external application code name.
    """

    def __init__(self, app_name, *args, **kwargs):
        super().__init__(
            f"The external application code name '{app_name}' is invalid.", *args, **kwargs
        )


class OAuthProviderError(Exception):
    """Indicate that the specified OAuth 2.0 provider does not exist or is not supported.

    Raised when a nonexistent or invalid OAuth 2.0 provider is specified during an operation on
    OAuth credentials.

    Arguments:
        provider_id: The ID of the nonexistent or unsupported OAuth provider as a string.
    """

    def __init__(self, provider_id, *args, **kwargs):
        super().__init__(
            f"The OAuth 2.0 provider '{provider_id}' does not exist or is not supported.",
            *args,
            **kwargs,
        )


def _app_name_to_provider_id(app_name):
    """Map the name of an external application to the ID of its provider.

    e.g. "google_drive" maps to "google"

    Arguments:
        app_name: The code name of the external application as a string.

    Raises:
        OAuthAppError: The specified external application code name is invalid.

    Returns:
        The ID of the OAuth provider that provides the specified application as a string.
    """

    provider_id = None

    if app_name == "dropbox":
        provider_id = "dropbox"
    elif app_name == "google_drive":
        provider_id = "google"
    else:
        raise OAuthAppError(app_name)

    return provider_id


def _provider_id_to_app_name(provider):
    """Map an OAuth provider ID to the name of the external application the provider provides.

    This mapping functionality is implemented as a function to make it easier to change the
    implementation in the future. Once we add a few more OAuth providers, we may want to add an
    OAuth provider table to the database. Things will get really complicated once we have the same
    provider providiging multiple external applications (e.g. Google Drive, Google Calendar).

    Arguments:
        provider: The ID of the provider as a string.

    Raises:
        OAuthProviderError: The specified provider is invalid.

    Returns:
        The code name of the external application as a string.
    """

    app_name = None

    if provider == "dropbox":
        app_name = "dropbox"
    elif provider == "google":
        app_name = "google_drive"
    else:
        raise OAuthProviderError(provider)

    return app_name


class Credential(db.Model):
    """Store encrypted OAuth tokens in the database.

    Attributes:
        access_token: An OAuth access token.
        expiry: The date and time (with time zone) at which the access token expires.
        provider_id: The name of the provider as a string.
        refresh_token: An OAuth refresh token.
        token_type: Always "bearer".
        user_id: The user_id of the user who owns the OAuth token.
    """

    __tablename__ = "credentials"
    __table_args__ = {"extend_existing": True, "schema": "oauth"}

    id = db.Column(db.Integer, primary_key=True)
    access_token = db.Column(
        StringEncryptedType(db.String, secret_key, AesEngine, "pkcs5"), nullable=False
    )
    provider_id = db.Column(db.String(256), nullable=False)
    expiry = db.Column(db.DateTime(timezone=True), nullable=False)
    refresh_token = db.Column(StringEncryptedType(db.String, secret_key, AesEngine, "pkcs5"))
    token_type = db.Column(db.String(128), nullable=False)
    user_id = db.Column(db.ForeignKey("users.user_id"), nullable=False)

    def refresh(self, cleanup=True, force=False):
        """Use this credential's refresh token to refresh the access token.

        This method modifies the instance on which it is called in place.

        Arguments:
            cleanup: A boolean indicating whether or not to delete a credential that cannot be
                refreshed from the database.
            force: A boolean indiciating whether or not to refresh the access token even if its
                expiration date has not yet passed.

        Raises:
            OAuthProviderError: This instance's provider_id attribute specifies an OAuth 2.0
                provider that either doesn't exist or is not supported.
            InvalidGrantError: The credential cannot be refreshed because the refresh token has
                been revoked or has expired.
        """

        if self.provider_id == "dropbox":
            # We store each token's expiration as a timezone-aware UTC timestamp. The Dropbox API
            # expects to receive a timezone-naive timestamp. We have to strip the timezone data off
            # the expiration timestamp before we instantiate the Dropbox client.
            dropbox = Dropbox(
                oauth2_access_token=self.access_token,
                oauth2_refresh_token=self.refresh_token,
                oauth2_access_token_expiration=self.expiry.replace(tzinfo=None),
                app_key=current_app.config["DROPBOX_APP_KEY"],
                app_secret=current_app.config["DROPBOX_APP_SECRET"],
            )

            getattr(dropbox, "{}refresh_access_token".format("" if force else "check_and_"))()

            # pylint: disable=protected-access
            self.access_token = dropbox._oauth2_access_token
            self.expiry = dropbox._oauth2_access_token_expiration
            assert self.refresh_token == dropbox._oauth2_refresh_token
            # pylint: enable=protected-access
        elif self.provider_id == "google":
            if force or self.expiry <= datetime.now(timezone.utc):
                flow = Flow.from_client_config(
                    current_app.config["GOOGLE_CLIENT_SECRET_OBJECT"], None
                )

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
        else:
            raise OAuthProviderError(self.provider_id)

        assert self.expiry.tzinfo is not None
        assert self.expiry.tzinfo.utcoffset(self.expiry) is not None

        db.session.add(self)
        db.session.commit()

    def revoke(self, cleanup=True):
        """Revoke the credential against the appropriate OAuth provider's servers.

        Arguments:
            cleanup: A boolean indicating whether or not to delete the credential from the
                database.
        """

        if self.provider_id == "dropbox":
            dropbox = Dropbox(
                oauth2_access_token=self.access_token,
                oauth2_refresh_token=self.refresh_token,
                oauth2_access_token_expiration=self.expiry.replace(tzinfo=None),
                app_key=current_app.config["DROPBOX_APP_KEY"],
                app_secret=current_app.config["DROPBOX_APP_SECRET"],
            )

            dropbox.auth_token_revoke()

            fractal_logger.info(
                f"successfully revoked Dropbox access for user '{self.user_id}'.",
                extra={"label": self.user_id},
            )
        elif self.provider_id == "google":
            response = requests.post(
                "https://oauth2.googleapis.com/revoke",
                params={"token": self.refresh_token},
                headers={"Content-Type": "application/x-www-form-urlencoded"},
            )

            if response.ok:
                fractal_logger.info(
                    f"Successfully revoked Google Drive access for user '{self.user_id}'.",
                    extra={"label": self.user_id},
                )
            else:
                fractal_logger.warning(
                    (
                        "Encountered an error while attempting to Google Drive access for user "
                        f"'{self.user_id}': {response.text}"
                    ),
                    extra={"label": self.user_id},
                )
        else:
            raise OAuthProviderError(self.provider_id)

        if cleanup:
            db.session.delete(self)
            db.session.commit()
