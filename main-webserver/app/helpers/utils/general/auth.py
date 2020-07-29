from app.imports import *
from app.helpers.utils.general.sql_commands import *


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
                    output = fractalSQLSelect(
                        table_name="v_ms", params={"vm_name": vm_name}
                    )
                    if output["success"] and output["rows"]:
                        username = output["rows"][0]["username"]
                elif "disk_name" in body.keys():
                    disk_name = body["disk_name"]
                    output = fractalSQLSelect(
                        table_name="disks", params={"disk_name": disk_name}
                    )
                    if output["success"] and output["rows"]:
                        username = output["rows"][0]["username"]
            elif request.method == "GET":
                username = request.args.get("username")
        except Exception as e:
            return (
                jsonify(
                    {"error": "No username provided, cannot authorize Bearer token."}
                ),
                UNAUTHORIZED,
            )

        current_user = get_jwt_identity()

        if (
            current_user != username
            and not os.getenv("DASHBOARD_USERNAME") in current_user
        ):
            print(
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


from app.imports import *
from app.helpers.utils.general.sql_commands import *


def adminRequired(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        # username = None

        # try:
        #     if request.method == "POST":
        #         username = body["username"]
        #     elif request.method == "GET":
        #         username = request.args.get("username")
        # except Exception as e:
        #     return (
        #         jsonify(
        #             {"error": "No username provided, cannot authorize Bearer token."}
        #         ),
        #         UNAUTHORIZED,
        #     )

        current_user = get_jwt_identity()
        if not os.getenv("DASHBOARD_USERNAME") in current_user:
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
