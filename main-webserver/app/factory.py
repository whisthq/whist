from .imports import *
from .celery_utils import init_celery

PKG_NAME = os.path.dirname(os.path.realpath(__file__)).split("/")[-1]

def create_app(app_name=PKG_NAME, **kwargs):
    app = Flask(app_name)
    jwtManager = JWTManager(app)
    if kwargs.get("celery"):
        init_celery(kwargs.get("celery"), app)
    log = logging.getLogger('werkzeug')
    log.disabled = True
    app.logger.disabled = True
    return (app, jwtManager)

def init_app(app):
    from app.vm_blueprint import vm_bp
    from app.account_blueprint import account_bp
    from app.stripe_blueprint import stripe_bp
    app.register_blueprint(vm_bp)
    app.register_blueprint(account_bp)
    app.register_blueprint(stripe_bp)
    return app

