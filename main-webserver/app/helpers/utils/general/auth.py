from app.imports import *

from app.helpers.utils.general.logs import *

from app.models.public import *
from app.models.hardware import *

from app.serializers.public import *
from app.serializers.hardware import *


def fractalAuth(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        username = None

        try:
            if request.method == "POST":
                body = json.loads(request.data)
                if "username" in body.keys():
                    username = body["username"]
                elif "email" in body.keys():
                    username = body["email"]
                elif "vm_name" in body.keys():
                    vm_name = body["vm_name"]

                    vm = UserVM.query.get(vm_name).first()

                    if vm:
                        username = vm.user_id
                elif "disk_name" in body.keys():
                    disk_name = body["disk_name"]

                    disk = OSDisk.query.get(disk_name)

                    if disk:
                        username = disk.user_id
            elif request.method == "GET":
                username = request.args.get("username")
        except Exception as e:
            fractalLog(
                function="",
                label="",
                logs="Bearer error: {error}".format(error=str(e)),
                level=logging.ERROR,
            )
            return (
                jsonify(
                    {"error": "No username provided, cannot authorize Bearer token."}
                ),
                UNAUTHORIZED,
            )

        current_user = get_jwt_identity()

        if current_user != username and not DASHBOARD_USERNAME in current_user:
            format = "%(asctime)s %(levelname)-8s [%(filename)s:%(lineno)d] %(message)s"

            logging.basicConfig(format=format, datefmt="%b %d %H:%M:%S")
            logger = logging.getLogger(__name__)
            logger.setLevel(logging.DEBUG)

            logger.info(
                "Authorization failed. Provided username {username} does not match username associated with provided Bearer token {bearer}.".format(
                    username=str(username), bearer=str(current_user)
                )
            )
            return (
                jsonify(
                    {
                        "error": "Authorization failed. Provided username {username} does not match username associated with provided Bearer token {bearer}.".format(
                            username=str(username), bearer=str(current_user)
                        )
                    }
                ),
                UNAUTHORIZED,
            )

        return f(*args, **kwargs)

    return wrapper


def adminRequired(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        current_user = get_jwt_identity()
        if not DASHBOARD_USERNAME in current_user:
            return (
                jsonify(
                    {
                        "error": "Authorization failed. Provided username does not match username associated with provided Bearer token."
                    }
                ),
                UNAUTHORIZED,
            )

        return f(*args, **kwargs)

    return wrapper
