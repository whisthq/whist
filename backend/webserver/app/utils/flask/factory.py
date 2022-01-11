import os

from http import HTTPStatus
from urllib.parse import urlunsplit
from typing import Any, Dict, Tuple

import stripe

from flask import current_app, Flask, jsonify, Response
from flask_cors import CORS
from flask_jwt_extended import jwt_required, JWTManager
from flask_jwt_extended.default_callbacks import default_unauthorized_callback
from jwt import PyJWKClient
from sqlalchemy.ext.declarative import DeferredReflection

from app.utils.general.logs import whist_logger
from app.config import CONFIG_MATRIX
from app.utils.flask.sentry import init_and_ensure_sentry_connection
from app.utils.metrics.flask_view import register_flask_view_metrics_monitor
from app.constants import env_names
from app.database.models.cloud import db

from app.utils.auth.auth0 import ScopeError
from app.utils.stripe.payments import payment_portal_factory, get_customer_id, PaymentRequired

jwtManager = JWTManager()


@jwtManager.decode_key_loader  # type: ignore
def decode_key_callback(
    unverified_headers: Dict[str, Any], _unverified_payload: Dict[str, Any]
) -> Any:
    """Dynamically determine the key to use to validate each JWT.

    The AUTH0_DOMAIN Flask configuration variable MUST be set.

    Inspiration:
        https://pyjwt.readthedocs.io/en/stable/usage.html#retrieve-rsa-signing-keys-from-a-jwks-endpoint

    Specification:
        https://flask-jwt-extended.readthedocs.io/en/stable/api/#flask_jwt_extended.JWTManager.decode_key_loader
    """

    parts = ("https", current_app.config["AUTH0_DOMAIN"], "/.well-known/jwks.json", None, None)
    jwks_client = PyJWKClient(urlunsplit(parts))

    # Extract the ID of the public verification key to download from the Auth0 tenant's
    # /.well-known/jwks.json endpoint from the access token headers. Then we can tell our
    # PyJWKClient instance to download the correct public key set for us and return its string
    # representation.
    return jwks_client.get_signing_key(unverified_headers["kid"]).key


def create_app(testing: bool = False) -> Flask:
    """A Flask application factory.

    See https://flask.palletsprojects.com/en/1.1.x/patterns/appfactories/?highlight=factory.

    Args:
        testing: A boolean indicating whether or not to configure the application for testing.

    Returns:
        A Flask application instance.
    """

    app = Flask(__name__.split(".", maxsplit=1)[0])

    # We want to look up CONFIG_MATRIX.location.action
    action = "test" if testing else "serve"
    location = "deployment" if ("DYNO" in os.environ or "GHA_FLASK_CLI" in os.environ) else "local"
    config_factory = getattr(getattr(CONFIG_MATRIX, location), action)
    config = config_factory()
    whist_logger.info("config_table = {}".format(config.config_table))

    app.config.from_object(config)
    whist_logger.info("environment = {}".format(app.config["ENVIRONMENT"]))

    # Only set up a connection to a Sentry event ingestion endpoint in production, staging, and development.
    if app.config["ENVIRONMENT"] in (
        env_names.PRODUCTION,
        env_names.STAGING,
        env_names.DEVELOPMENT,
    ):
        init_and_ensure_sentry_connection(app.config["ENVIRONMENT"], app.config["SENTRY_DSN"])

    # Set the Stripe API key.
    stripe.api_key = app.config["STRIPE_SECRET"]

    from app.utils.general.limiter import limiter

    limiter.init_app(app)
    db.init_app(app)
    jwtManager.init_app(app)
    register_flask_view_metrics_monitor(app)

    CORS(app)

    with app.app_context():
        DeferredReflection.prepare(db.engine)

    register_handlers(app)
    register_blueprints(app)

    return app


def register_handlers(app: Flask) -> None:
    """
    Register all Flask app handlers. This should SHOULD NOT include endpoint definitions, which
    happens via blueprints in `register_blueprints`. These handlers should be other functions
    that need to be decorated by a Flask app (such as app.before_request(func)).
    """
    from app.utils.flask.flask_handlers import can_process_requests_handler

    can_process_requests_handler(app)

    @app.errorhandler(ScopeError)
    def _handle_scope_error(e: Any) -> Any:
        return default_unauthorized_callback(str(e))

    @app.errorhandler(PaymentRequired)
    def _handle_payment_required(_error: Any) -> Tuple[Response, HTTPStatus]:
        return (
            jsonify(error="That resource is only available to Whist subscribers"),
            HTTPStatus.PAYMENT_REQUIRED,
        )


def register_blueprints(app: Flask) -> None:
    """
    Registers all blueprints (endpoints) for the Flask app

    Args:
        - app: Flask object
    """

    from app.api.mandelbox import aws_mandelbox_bp
    from app.api.regions import aws_region_bp

    app.register_blueprint(aws_mandelbox_bp)
    app.register_blueprint(aws_region_bp)

    app.add_url_rule(
        "/payment_portal_url", view_func=jwt_required()(payment_portal_factory(get_customer_id))
    )
