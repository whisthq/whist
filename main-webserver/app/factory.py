from .imports import *
from .celery_utils import init_celery

PKG_NAME = os.path.dirname(os.path.realpath(__file__)).split("/")[-1]

def create_app(app_name=PKG_NAME, **kwargs):
    app = Flask(app_name)
    if kwargs.get("celery"):
        init_celery(kwargs.get("celery"), app)
    from app.vm_blueprint import vm_bp
    from app.account_blueprint import account_bp
    app.register_blueprint(vm_bp)
    app.register_blueprint(account_bp)
    return app
