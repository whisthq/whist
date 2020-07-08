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
    from .blueprints.account_blueprint import account_bp
    from .blueprints.token_blueprint import token_bp
    from .blueprints.azure_vm_blueprint import azure_vm_bp
    from .blueprints.celery_status_blueprint import celery_status_bp
    from .blueprints.azure_disk_blueprint import azure_disk_bp
    from .blueprints.artifact_blueprint import artifact_bp
    from .blueprints.mail_blueprint import mail_bp


    app.register_blueprint(account_bp)
    app.register_blueprint(token_bp)
    app.register_blueprint(azure_vm_bp)
    app.register_blueprint(celery_status_bp)
    app.register_blueprint(azure_disk_bp)
    app.register_blueprint(artifact_bp)
    app.register_blueprint(mail_bp)

    return app
