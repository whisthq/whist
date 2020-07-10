from .imports import *
from .celery_utils import init_celery

PKG_NAME = os.path.dirname(os.path.realpath(__file__)).split("/")[-1]


def create_app(app_name=PKG_NAME, **kwargs):
    template_dir = os.path.dirname(os.path.realpath(__file__))
    template_dir = os.path.join(template_dir, "templates")

    app = Flask(app_name, template_folder=template_dir)

    jwtManager = JWTManager(app)

    if kwargs.get("celery"):
        init_celery(kwargs.get("celery"), app)

    return (app, jwtManager)


def init_app(app):
    from .blueprints.admin.report_blueprint import report_bp
    from .blueprints.admin.analytics_blueprint import analytics_bp
    from .blueprints.admin.admin_blueprint import admin_bp
    from .blueprints.admin.sql_table_blueprint import table_bp
    from .blueprints.admin.logs_blueprint import logs_bp

    from .blueprints.auth.account_blueprint import account_bp
    from .blueprints.auth.token_blueprint import token_bp
    from .blueprints.auth.google_auth_blueprint import google_auth_bp

    from .blueprints.celery.celery_status_blueprint import celery_status_bp

    from .blueprints.azure.azure_vm_blueprint import azure_vm_bp
    from .blueprints.azure.azure_disk_blueprint import azure_disk_bp
    from .blueprints.azure.artifact_blueprint import artifact_bp

    from .blueprints.mail.mail_blueprint import mail_bp
    from .blueprints.mail.newsletter_blueprint import newsletter_bp

    from .blueprints.payment.stripe_blueprint import stripe_bp

    app.register_blueprint(account_bp)
    app.register_blueprint(token_bp)
    app.register_blueprint(azure_vm_bp)
    app.register_blueprint(celery_status_bp)
    app.register_blueprint(azure_disk_bp)
    app.register_blueprint(google_auth_bp)
    app.register_blueprint(artifact_bp)
    app.register_blueprint(mail_bp)
    app.register_blueprint(newsletter_bp)
    app.register_blueprint(stripe_bp)
    app.register_blueprint(report_bp)
    app.register_blueprint(analytics_bp)
    app.register_blueprint(admin_bp)
    app.register_blueprint(table_bp)
    app.register_blueprint(logs_bp)

    return app
