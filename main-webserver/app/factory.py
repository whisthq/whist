from .imports import *
from .celery_utils import init_celery
import click

PKG_NAME = os.path.dirname(os.path.realpath(__file__)).split("/")[-1]

def create_app(app_name=PKG_NAME, **kwargs):
    app = Flask(app_name)
    jwtManager = JWTManager(app)
    if kwargs.get("celery"):
        init_celery(kwargs.get("celery"), app)

    return (app, jwtManager)


def init_app(app):
    from .blueprints.account_blueprint import account_bp

    app.register_blueprint(account_bp)

    return app
