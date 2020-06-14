from .imports import *
from .celery_utils import init_celery

PKG_NAME = os.path.dirname(os.path.realpath(__file__)).split("/")[-1]


def create_app(app_name=PKG_NAME, **kwargs):
    app = Flask(app_name)
    jwtManager = JWTManager(app)
    if kwargs.get("celery"):
        init_celery(kwargs.get("celery"), app)

    return (app, jwtManager)


def init_app(app):
    from .blueprints.account_blueprint import account_bp
    from .blueprints.token_blueprint import token_bp
    from .blueprints.azure_vm_blueprint import azure_vm_bp
    from .blueprints.celery_status_blueprint import celery_status_bp

    app.register_blueprint(account_bp)
    app.register_blueprint(token_bp)
    app.register_blueprint(azure_vm_bp)
    app.register_blueprint(celery_status_bp)

    return app
