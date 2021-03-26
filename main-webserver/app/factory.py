import os

import stripe

from flask import Flask
from flask_cors import CORS
from flask_jwt_extended import JWTManager
from flask_marshmallow import Marshmallow
from flask_sendgrid import SendGrid

from app.helpers.utils.general.logs import fractal_logger
from app.config import CONFIG_MATRIX
from app.sentry import init_and_ensure_sentry_connection
from app.helpers.utils.metrics.flask_view import register_flask_view_metrics_monitor
import app.constants.env_names as env_names

jwtManager = JWTManager()
ma = Marshmallow()
mail = SendGrid()


def create_app(testing=False):
    """A Flask application factory.

    See https://flask.palletsprojects.com/en/1.1.x/patterns/appfactories/?highlight=factory.

    Args:
        testing: A boolean indicating whether or not to configure the application for testing.

    Returns:
        A Flask application instance.
    """

    app = Flask(__name__.split(".")[0])

    # We want to look up CONFIG_MATRIX.location.action
    action = "test" if testing else "serve"
    location = "deployment" if "DYNO" in os.environ else "local"
    config_factory = getattr(getattr(CONFIG_MATRIX, location), action)
    config = config_factory()
    fractal_logger.info("config_table = {}".format(config.config_table))

    app.config.from_object(config)
    fractal_logger.info("environment = {}".format(app.config["ENVIRONMENT"]))

    # Only set up a connection to a Sentry event ingestion endpoint in production and staging.
    if app.config["ENVIRONMENT"] in (env_names.PRODUCTION, env_names.STAGING):
        init_and_ensure_sentry_connection(app.config["ENVIRONMENT"], app.config["SENTRY_DSN"])

    # Set the Stripe API key.
    stripe.api_key = app.config["STRIPE_SECRET"]

    from .models import db
    from .helpers.utils.general.limiter import limiter

    limiter.init_app(app)
    db.init_app(app)
    jwtManager.init_app(app)
    ma.init_app(app)
    mail.init_app(app)
    register_flask_view_metrics_monitor(app)

    CORS(app)

    register_handlers(app)
    register_blueprints(app)
    return app


def register_handlers(app: Flask):
    """
    Register all flask app handlers
    """
    from app.flask_handlers import can_process_requests_handler

    can_process_requests_handler(app)


def register_blueprints(app):
    """
    Registers all blueprints (endpoints) for the Flask app

    Args:
        - app: Flask object
    """

    from .blueprints.admin.admin_blueprint import admin_bp
    from .blueprints.admin.logs_blueprint import logs_bp
    from .blueprints.admin.hasura_blueprint import hasura_bp

    from .blueprints.auth.account_blueprint import account_bp
    from .blueprints.auth.token_blueprint import token_bp

    from .blueprints.celery.celery_status_blueprint import celery_status_bp

    from .blueprints.aws.aws_container_blueprint import aws_container_bp

    from .blueprints.mail.mail_blueprint import mail_bp
    from .blueprints.mail.newsletter_blueprint import newsletter_bp

    from .blueprints.payment.stripe_blueprint import stripe_bp

    from .blueprints.host_service.host_service_blueprint import host_service_bp

    from .blueprints.oauth import oauth_bp

    app.register_blueprint(account_bp)
    app.register_blueprint(token_bp)
    app.register_blueprint(celery_status_bp)
    app.register_blueprint(aws_container_bp)
    app.register_blueprint(hasura_bp)
    app.register_blueprint(mail_bp)
    app.register_blueprint(newsletter_bp)
    app.register_blueprint(stripe_bp)
    app.register_blueprint(admin_bp)
    app.register_blueprint(logs_bp)
    app.register_blueprint(host_service_bp)
    app.register_blueprint(oauth_bp)
