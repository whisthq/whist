from .imports import *
from .factory import *
from .utils import *
from .logger import *


def make_celery(app_name=__name__):
    backend = os.getenv("REDIS_URL")
    broker = os.getenv("REDIS_URL")
    return Celery(app_name, backend=backend, broker=broker)


def dispose_engine(engine):
    engine.dispose()


celery = make_celery()
engine = db.create_engine(os.getenv("DATABASE_URL"), echo=False, pool_pre_ping=True)
Session = sessionmaker(bind=engine, autocommit=False)

app, jwtManager = create_app(celery=celery)
gen = yieldNumber()


# class ContextFilter(logging.Filter):
#     hostname = socket.gethostname()

#     def filter(self, record):
#         record.hostname = ContextFilter.hostname
#         return True


# syslog = SysLogHandler(
#     address=(os.getenv("PAPERTRAIL_URL"), int(os.getenv("PAPERTRAIL_PORT")))
# )
# syslog.addFilter(ContextFilter())

# format = "%(asctime)s %(levelname)-8s [%(filename)s:%(lineno)d] %(message)s"
# # format = "%(asctime)s [%(pathname)s:%(lineno)d] %(message)s"
# formatter = logging.Formatter(format, datefmt="%b %d %H:%M:%S")
# syslog.setFormatter(formatter)
# # logging.setLoggerClass(MyLogger)

# logger = logging.getLogger("root")
# logger.setLevel(logging.INFO)
# logger.addHandler(syslog)
# logger.info("what the")


def generateID(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        kwargs["ID"] = next(gen)
        return f(*args, **kwargs)

    return wrapper


def logRequestInfo(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        try:
            vm_ip = None
            if request.headers.getlist("X-Forwarded-For"):
                vm_ip = request.headers.getlist("X-Forwarded-For")[0]
            else:
                vm_ip = request.remote_addr

            papertrail = True
            # if request.path in ['/vm/connectionStatus', '/vm/winlogonStatus']:
            # 	papertrail = False

            try:
                body = request.get_json()

                if body:
                    for k, v in body.items():
                        if isinstance(v, str):
                            body[k] = v[0 : min(100, len(v))]
            except Exception as e:
                body = {}

            sendDebug(
                kwargs["ID"],
                "[{}][{}][{}](BODY: {})".format(
                    request.path, request.method, str(vm_ip), str(body)
                ),
                papertrail=papertrail,
            )

        except Exception as e:
            print(str(e))
        return f(*args, **kwargs)

    return wrapper


app = init_app(app)

app.config["MAIL_SERVER"] = "ming@fractalcomputers.com"
app.config["MAIL_PORT"] = 465
app.config["MAIL_USE_SSL"] = True
app.config["JWT_SECRET_KEY"] = os.getenv("JWT_SECRET_KEY")

CORS(app)
