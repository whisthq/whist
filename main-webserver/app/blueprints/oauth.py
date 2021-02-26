"""A blueprint for endpoints that allow users to manage connected applications."""

import json

from collections import namedtuple
from urllib.parse import urljoin

import click

from dropbox import DropboxOAuth2Flow
from flask import abort, Blueprint, current_app, jsonify, redirect, request, session, url_for
from flask.views import MethodView
from flask_jwt_extended import get_jwt_identity, jwt_required
from google_auth_oauthlib.flow import Flow

from app.models import Credential, db, User
from app.serializers.oauth import CredentialSchema
from app.models.oauth import _app_name_to_provider_id, _provider_id_to_app_name
from app.helpers.utils.general.limiter import limiter, RATE_LIMIT_PER_MINUTE


oauth_bp = Blueprint("oauth", __name__, cli_group="credentials")
Token = namedtuple(
    "Token",
    ("access_token", "expiry", "refresh_token", "token_type"),
    defaults=(None, "bearer"),
)

oauth_bp.cli.help = "Manipulate encrypted OAuth credentials in the database."


class ConnectedAppsAPI(MethodView):
    """Manage connected external applications."""

    decorators = (jwt_required(),)

    def get(self):  # pylint: disable=no-self-use
        """Fetch the list of external apps that the user has connected to her Fractal account."""

        apps = []
        user = User.query.get(get_jwt_identity())

        assert user

        for credential in user.credentials:
            apps.append(_provider_id_to_app_name(credential.provider_id))

        return jsonify({"app_names": apps})

    def delete(self, app_name):  # pylint: disable=no-self-use
        """Revoke Fractal's authorization to use the specified app.

        Arguments:
            app_name: The name of the application for which to revoke Fractal's authorization.
        """

        credential = None
        user = User.query.get(get_jwt_identity())

        assert user

        for cred in user.credentials:
            if _provider_id_to_app_name(cred.provider_id) == app_name:
                credential = cred

        if not credential:
            abort(400)

        credential.revoke()

        # TODO: It may be appropriate to return a status code of 204 here. Doing so would require
        # some changes to be made to the client application's apiDelete function.
        return jsonify({})


@oauth_bp.route("/oauth/authorize")
@limiter.limit(RATE_LIMIT_PER_MINUTE)
@jwt_required()
def authorize():
    """Initiate the OAuth exchange by requesting an authorization grant.

    Query parameters:
        app: The code name of the external application to connect to the user's Fractal account.

    Returns:
        An HTTP redirection response to the authorization grant endpoint, per the OAuth
        specification.
    """

    try:
        app_name = request.args["external_app"]
    except KeyError:
        abort(400)

    callback_uri = url_for(".callback", _external=True)

    if app_name == "dropbox":
        flow = DropboxOAuth2Flow(
            current_app.config["DROPBOX_APP_KEY"],
            callback_uri,
            session,
            current_app.config["DROPBOX_CSRF_TOKEN_SESSION_KEY"],
            consumer_secret=current_app.config["DROPBOX_APP_SECRET"],
            token_access_type="offline",
        )
        auth_url = flow.start()
    elif app_name == "google_drive":
        flow = Flow.from_client_config(
            current_app.config["GOOGLE_CLIENT_SECRET_OBJECT"],
            ("https://www.googleapis.com/auth/drive",),
            redirect_uri=callback_uri,
        )
        auth_url, state = flow.authorization_url(
            access_type="offline", include_granted_scopes="true"
        )
        session["state"] = state
    else:
        abort(400)

    session["external_app"] = app_name
    session["user"] = get_jwt_identity()

    return redirect(auth_url)


@oauth_bp.route("/oauth/callback")
@limiter.limit(RATE_LIMIT_PER_MINUTE)
def callback():
    """Redeem an OAuth 2.0 authorization code for an access token.

    Query parameters:
        app: The code name of the external application to connect to the user's Fractal account.

    Returns:
        An HTTP redirection to the Fractal's website.
    """

    try:
        app_name = session["external_app"]
        user = session["user"]
    except KeyError:
        abort(400)

    callback_uri = url_for(".callback", _external=True)

    if app_name == "dropbox":
        flow = DropboxOAuth2Flow(
            current_app.config["DROPBOX_APP_KEY"],
            callback_uri,
            session,
            current_app.config["DROPBOX_CSRF_TOKEN_SESSION_KEY"],
            consumer_secret=current_app.config["DROPBOX_APP_SECRET"],
            token_access_type="offline",
        )
        result = flow.finish(request.args)
        token = Token(result.access_token, result.expires_at, result.refresh_token)
    elif app_name == "google_drive":
        try:
            state = session["state"]
        except KeyError:
            abort(400)

        flow = Flow.from_client_config(
            current_app.config["GOOGLE_CLIENT_SECRET_OBJECT"],
            None,
            redirect_uri=callback_uri,
            state=state,
        )

        flow.fetch_token(authorization_response=request.url)

        token = Token(
            flow.credentials.token, flow.credentials.expiry, flow.credentials.refresh_token
        )
    else:
        abort(400)

    put_credential(user, _app_name_to_provider_id(app_name), token)

    return redirect(
        urljoin(
            current_app.config["FRONTEND_URL"],
            f"/auth/bypass?callback=fractal://oauth?successfully_authenticated={app_name}",
        )
    )


@oauth_bp.route("/external_apps")
@limiter.limit(RATE_LIMIT_PER_MINUTE)
def external_apps():
    """List metadata for all available external applications."""

    # TODO: Load this data from a table in the database.
    return jsonify(
        {
            "data": [
                {
                    "code_name": "dropbox",
                    "display_name": "Dropbox",
                    "image_s3_uri": "https://fractal-external-app-images.s3.amazonaws.com/dropbox.svg",  # pylint: disable=line-too-long
                    "tos_uri": "https://www.dropbox.com/terms",
                },
                {
                    "code_name": "google_drive",
                    "display_name": "Google Drive",
                    "image_s3_uri": "https://fractal-external-app-images.s3.amazonaws.com/google-drive-256.svg",  # pylint: disable=line-too-long
                    "tos_uri": "https://www.google.com/drive/terms-of-service/",
                },
            ]
        }
    )


@oauth_bp.cli.command("get", help="Show the token used to authenticate with PROVIDER as USER.")
@click.argument("user")
@click.argument("provider")
def get_credential(user, provider):
    """Show the token used to authenticate with PROVIDER as USER.

    Arguments:
        user: The user_id of the user whose credentials should be retrieved.
        provider: The provider_id of the OAuth provider with which the token is used to
            authenticate.
    """

    credentials = []
    user_row = User.query.get(user)
    schema = CredentialSchema(only=("access_token", "expiry", "refresh_token", "token_type"))

    if not user_row:
        raise click.ClickException(f"Could not find user '{user}'.")

    for credential in user_row.credentials:
        if credential.provider_id == provider:
            credentials.append(credential)

    if not credentials:
        raise click.ClickException(
            f"User '{user}' has not authorized Fractal to use '{provider}' on their behalf."
        )

    click.echo(json.dumps([schema.dump(credential) for credential in credentials], indent=2))


@oauth_bp.cli.command("revoke", help="Revoke token used to authenticate with PROVIDER as USER.")
@click.argument("user")
@click.argument("provider")
def revoke_credential(user, provider):
    """Revoke the token used to authenticate with PROVIDER as USER.

    Arguments:
        user: The user_id of the user whose credentials should be retrieved.
        provider: The provider_id of the OAuth provider against which the token should be revoked.
    """

    credentials = []
    user_row = User.query.get(user)

    if not user_row:
        raise click.ClickException(f"Could not find user '{user}'.")

    for credential in user_row.credentials:
        if credential.provider_id == provider:
            credentials.append(credential)

    if not credentials:
        raise click.ClickException(
            f"User '{user}' has not authorized Fractal to use '{provider}' on their behalf."
        )

    for credential in credentials:
        credential.revoke()

    click.echo(f"Successfully revoked fractal's access to '{provider}' on behalf of '{user}'.")


def put_credential(user_id, provider_id, token):
    """Add a new credential to the database or update an existing one.

    Arguments:
        user_id: The user ID of the user who owns this credential.
        provider_id: The ID of the OAuth 2.0 provider as a string.
        token: An instance of the Token namedtuple wrapper.

    Returns:
        An instance of the Credential data model.
    """

    credential = None
    user = User.query.get(user_id)

    assert user

    for cred in user.credentials:
        # Try to update an existing credential.
        if cred.provider_id == provider_id:
            if not credential:
                credential = cred
                cred.access_token = token.access_token
                cred.expiry = token.expiry

                if token.refresh_token:
                    cred.refresh_token = token.refresh_token

                db.session.add(cred)
            else:
                # Delete any other entries that have the same provider_id.
                db.session.delete(cred)

    if not credential:
        # Insert a new credential.
        credential = Credential(
            user_id=user.user_id,
            access_token=token.access_token,
            token_type=token.token_type,
            expiry=token.expiry,
            provider_id=provider_id,
            refresh_token=token.refresh_token,
        )

        db.session.add(credential)

    db.session.commit()

    return credential


connected_apps = ConnectedAppsAPI.as_view("connected_apps")

oauth_bp.add_url_rule("/connected_apps", view_func=connected_apps, methods=("GET",))
oauth_bp.add_url_rule(
    "/connected_apps/<string:app_name>", view_func=connected_apps, methods=("DELETE",)
)
