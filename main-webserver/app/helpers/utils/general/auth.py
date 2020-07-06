from app.imports import *


def fractalAuth(f):
    @wraps(f)
    def wrapper(*args, **kwargs):
        received_from = (
            request.headers.getlist("X-Forwarded-For")[0]
            if request.headers.getlist("X-Forwarded-For")
            else request.remote_addr
        )

        username = None

        try:
            if request.method == "POST":
                username = json.loads(request.data)["username"]
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
        if current_user != username:
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
