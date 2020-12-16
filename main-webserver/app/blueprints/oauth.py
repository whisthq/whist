"""A blueprint for endpoints that allow users to manage connected applications."""

from collections import namedtuple
from urllib.parse import urljoin

import click

from flask import abort, Blueprint, current_app, jsonify, redirect, request, session, url_for
from flask.views import MethodView
from flask_jwt_extended import get_jwt_identity, jwt_required
from google_auth_oauthlib.flow import Flow

from app.models import Credential, db, User
from app.serializers.oauth import CredentialSchema

oauth_bp = Blueprint("oauth", __name__, cli_group="credentials")
Token = namedtuple(
    "Token",
    ("access_token", "expiry", "refresh_token", "token_type"),
    defaults=(None, "bearer"),
)

oauth_bp.cli.help = "Manipulate encrypted OAuth credentials in the database."


class ConnectedAppsAPI(MethodView):
    """Manage connected external applications."""

    decorators = (jwt_required,)

    def get(self):  # pylint: disable=no-self-use
        """Fetch the list of external apps that the user has connected to her Fractal account."""

        apps = []
        user = User.query.get(get_jwt_identity())

        assert user

        if user.credentials:
            apps.append("google_drive")

        return jsonify({"app_names": apps})

    def delete(self, app_name):  # pylint: disable=no-self-use
        """Revoke Fractal's authorization to use the specified app.

        Arguments:
            app_name: The name of the application for which to revoke Fractal's authorization.
        """

        user = User.query.get(get_jwt_identity())

        assert user

        if not (user.credentials and app_name == "google_drive"):
            abort(400)

        for credential in user.credentials:
            credential.revoke()

        # TODO: It may be appropriate to return a status code of 204 here. Doing so would require
        # some changes to be made to the client application's apiDelete function.
        return jsonify({})


@oauth_bp.route("/oauth/authorize")
@jwt_required
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

    # TODO: Support external applications other than Google drive.
    assert app_name == "google_drive"

    callback_uri = url_for(".callback", _external=True)
    flow = Flow.from_client_config(
        current_app.config["GOOGLE_CLIENT_SECRET_OBJECT"],
        ("https://www.googleapis.com/auth/drive",),
        redirect_uri=callback_uri,
    )
    auth_url, state = flow.authorization_url(access_type="offline", include_granted_scopes="true")

    session["external_app"] = app_name
    session["state"] = state
    session["user"] = get_jwt_identity()

    return redirect(auth_url)


@oauth_bp.route("/oauth/callback")
def callback():
    """Redeem an OAuth 2.0 authorization code for an access token.

    Query parameters:
        app: The code name of the external application to connect to the user's Fractal account.

    Returns:
        An HTTP redirection to the Fractal's website.
    """

    try:
        app_name = session["external_app"]
        state = session["state"]
        user = session["user"]
    except KeyError:
        abort(400)

    # TODO: Support external applications other than Google drive.
    assert app_name == "google_drive"

    callback_uri = url_for(".callback", _external=True)
    flow = Flow.from_client_config(
        current_app.config["GOOGLE_CLIENT_SECRET_OBJECT"],
        None,
        redirect_uri=callback_uri,
        state=state,
    )

    flow.fetch_token(authorization_response=request.url)

    token = Token(flow.credentials.token, flow.credentials.expiry, flow.credentials.refresh_token)

    put_credential(user, token)

    return redirect(
        urljoin(
            current_app.config["FRONTEND_URL"],
            "/auth/bypass?callback=fractal://oauth?successfully_authenticated=google_drive",
        )
    )


@oauth_bp.route("/external_apps")
def external_apps():
    """List metadata for all available external applications."""

    # TODO: Load this data from a table in the database.
    return jsonify(
        {
            "data": [
                {
                    "code_name": "google_drive",
                    "display_name": "Google Drive",
                    "image_s3_uri": "https://fractal-external-app-images.s3.amazonaws.com/google-drive-256.svg",  # pylint: disable=line-too-long
                    "tos_uri": "https://www.google.com/drive/terms-of-service/",
                }
            ]
        }
    )


@oauth_bp.cli.command("get", help="Show the credentials owned by the user with user_id USER_ID.")
@click.argument("user_id")
def get_credential(user_id):
    """Show the credentials owned by the user with user_id USER_ID.

    Arguments:
        user_id: The user_id of the user whose credentials should be retrieved.
    """

    user = User.query.get(user_id)
    schema = CredentialSchema()

    if not user:
        raise click.ClickException(f"Could not find user '{user.user_id}'.")

    credentials = user.credentials

    if not credentials:
        raise click.ClickException(
            f"User '{user.user_id}' has not connected any applications to their Fractal account."
        )

    assert len(credentials) == 1

    click.echo(schema.dumps(credentials[0], indent=2))


def put_credential(user_id, token):
    """Add a new credential to the database or update an existing one.

    Arguments:
        user_id: The user ID of the user who owns this credential.
        token: An instance of the Token namedtuple wrapper.

    Returns:
        An instance of the Credential data model.
    """

    credential = None
    user = User.query.get(user_id)

    assert user

    credentials = user.credentials

    if credentials and not token.refresh_token:
        assert len(credentials) == 1

        credential = credentials[0]
        credential.access_token = token.access_token
        credential.expiry = token.expiry

        assert token.token_type == credential.token_type
    else:
        # This branch of the conditional will be run if either the user's list of credentials is
        # empty or the new token's refresh token differs from an existing token's refresh token and
        # is not blank. In the former case, the loop below has no effect. In the latter case, we
        # can consider the existing token or tokens stale and delete them; no user should have more
        # than one OAuth token associated with her account at a time because there is only one
        # supported external application.
        for cred in credentials:
            db.session.delete(cred)

        credential = Credential(
            user_id=user.user_id,
            access_token=token.access_token,
            token_type=token.token_type,
            expiry=token.expiry,
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
