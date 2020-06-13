from .imports import *
from .factory import *
from .helpers.utils.logs import *


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
            body = None

        kwargs["body"] = body
        kwargs["received_from"] = received_from

        return f(*args, **kwargs)

    return wrapper


celery = make_celery()
engine = db.create_engine(os.getenv("DATABASE_URL"), echo=False, pool_pre_ping=True)
Session = sessionmaker(bind=engine, autocommit=False)

app, jwtManager = create_app(celery=celery)
app = init_app(app)

app.config["MAIL_SERVER"] = "ming@fractalcomputers.com"
app.config["MAIL_PORT"] = 465
app.config["MAIL_USE_SSL"] = True
app.config["JWT_SECRET_KEY"] = os.getenv("JWT_SECRET_KEY")

CORS(app)
