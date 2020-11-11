import os

import sentry_sdk

from flask import Flask
from flask_cors import CORS
from flask_jwt_extended import JWTManager
from flask_marshmallow import Marshmallow
from flask_sendgrid import SendGrid
from sentry_sdk.integrations.flask import FlaskIntegration
from sentry_sdk.integrations.celery import CeleryIntegration

from .celery_utils import init_celery

PKG_NAME = os.path.dirname(os.path.realpath(__file__)).split("/")[-1]

jwtManager = JWTManager()
ma = Marshmallow()
mail = SendGrid()


def create_app(app_name=PKG_NAME, **kwargs):
    if os.getenv("USE_PRODUCTION_KEYS").upper() == "true":
        env = "prod"
    else:
        env = "staging"
    sentry_sdk.init(
        dsn="https://3d228295baab4919a7e4fa8163c72098@o400459.ingest.sentry.io/5394545",
        integrations=[FlaskIntegration(), CeleryIntegration()],
        environment=env,
        release="main-webserver@" + os.getenv("HEROKU_SLUG_COMMIT", "local"),
    )

    template_dir = os.path.dirname(os.path.realpath(__file__))
    template_dir = os.path.join(template_dir, "templates")

    from .constants.config import (
        DATABASE_URL,
        JWT_SECRET_KEY,
        SENDGRID_API_KEY,
    )

    app = Flask(app_name, template_folder=template_dir)
    app.config["JWT_SECRET_KEY"] = JWT_SECRET_KEY
    app.config["ROOT_DIRECTORY"] = os.path.dirname(os.path.abspath(__file__))
    app.config["SENDGRID_API_KEY"] = SENDGRID_API_KEY
    app.config["SENDGRID_DEFAULT_FROM"] = "noreply@tryfractal.com"
    app.config["SQLALCHEMY_DATABASE_URI"] = DATABASE_URL
    app.config["SQLALCHEMY_TRACK_MODIFICATIONS"] = False

    if kwargs.get("celery"):
        init_celery(kwargs.get("celery"), app)

    from .models import db

    db.init_app(app)
    jwtManager.init_app(app)
    ma.init_app(app)
    mail.init_app(app)

    CORS(app)

    return init_app(app)


def init_app(app):
    from .blueprints.admin.report_blueprint import report_bp
    from .blueprints.admin.analytics_blueprint import analytics_bp
    from .blueprints.admin.admin_blueprint import admin_bp
    from .blueprints.admin.sql_table_blueprint import table_bp
    from .blueprints.admin.logs_blueprint import logs_bp
    from .blueprints.admin.hasura_blueprint import hasura_bp

    from .blueprints.auth.account_blueprint import account_bp
    from .blueprints.auth.token_blueprint import token_bp
    from .blueprints.auth.google_auth_blueprint import google_auth_bp

    from .blueprints.celery.celery_status_blueprint import celery_status_bp

    from .blueprints.aws.aws_container_blueprint import aws_container_bp

    from .blueprints.mail.mail_blueprint import mail_bp
    from .blueprints.mail.newsletter_blueprint import newsletter_bp

    from .blueprints.payment.stripe_blueprint import stripe_bp

    from .blueprints.host_service.host_service_blueprint import host_service_bp

    app.register_blueprint(account_bp)
    app.register_blueprint(token_bp)
    app.register_blueprint(celery_status_bp)
    app.register_blueprint(aws_container_bp)
    app.register_blueprint(hasura_bp)
    app.register_blueprint(google_auth_bp)
    app.register_blueprint(mail_bp)
    app.register_blueprint(newsletter_bp)
    app.register_blueprint(stripe_bp)
    app.register_blueprint(report_bp)
    app.register_blueprint(analytics_bp)
    app.register_blueprint(admin_bp)
    app.register_blueprint(table_bp)
    app.register_blueprint(logs_bp)
    app.register_blueprint(host_service_bp)

    return app
