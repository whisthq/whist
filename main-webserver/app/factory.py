from .imports import *
from .celery_utils import init_celery
import click

PKG_NAME = os.path.dirname(os.path.realpath(__file__)).split("/")[-1]


def create_app(app_name=PKG_NAME, **kwargs):
    template_dir = os.path.dirname(
        os.path.dirname(os.path.abspath(os.path.dirname(__file__)))
    )
    template_dir = os.path.join(template_dir, "vm-webserver")
    template_dir = os.path.join(template_dir, "app")
    template_dir = os.path.join(template_dir, "templates")
    print(template_dir)
    app = Flask(app_name, template_folder=template_dir)
    jwtManager = JWTManager(app)
    if kwargs.get("celery"):
        init_celery(kwargs.get("celery"), app)

    # log = logging.getLogger("werkzeug")
    # log.setLevel(logging.ERROR)

    return (app, jwtManager)


def init_app(app):
    from .blueprints.vm_blueprint import vm_bp
    from .blueprints.account_blueprint import account_bp
    from .blueprints.stripe_blueprint import stripe_bp
    from .blueprints.p2p_blueprint import p2p_bp
    from .blueprints.disk_blueprint import disk_bp
    from .blueprints.mail_blueprint import mail_bp
    from .blueprints.report_blueprint import report_bp
    from .blueprints.version_blueprint import version_bp

    app.register_blueprint(vm_bp)
    app.register_blueprint(account_bp)
    app.register_blueprint(stripe_bp)
    app.register_blueprint(p2p_bp)
    app.register_blueprint(disk_bp)
    app.register_blueprint(mail_bp)
    app.register_blueprint(report_bp)
    app.register_blueprint(version_bp)

    return app
