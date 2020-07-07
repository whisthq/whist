from .imports import *
from .factory import *

from .helpers.utils.general.logs import *
from .helpers.utils.general.sql_commands import *
from .helpers.utils.general.tokens import *
from .helpers.utils.general.time import *
from .helpers.utils.general.auth import *


def make_celery(app_name=__name__):
    backend = os.getenv("REDIS_URL")
    broker = os.getenv("REDIS_URL")
    return Celery(app_name, backend=backend, broker=broker)


def fractalPreProcess(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        received_from = (
            request.headers.getlist("X-Forwarded-For")[0]
            if request.headers.getlist("X-Forwarded-For")
            else request.remote_addr
        )

        try:
            body = json.loads(request.data) if request.method == "POST" else None
        except Exception as e:
            print(str(e))
            body = None

        kwargs["body"] = body
        kwargs["received_from"] = received_from

        return f(*args, **kwargs)

    return wrapper


# fractalLog(function="__init__", label="None", logs="Initializating server")

celery_instance = make_celery()

app, jwtManager = create_app(celery=celery_instance)
app = init_app(app)

app.config["JWT_SECRET_KEY"] = os.getenv("JWT_SECRET_KEY")
app.config["ROOT_DIRECTORY"] = os.path.dirname(os.path.abspath(__file__))

CORS(app)
