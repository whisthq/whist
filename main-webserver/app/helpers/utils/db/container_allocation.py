import uuid
from flask.json import jsonify
from pydantic import BaseModel
from flask_pydantic import validate
from _meta import db
from app.models.hardware import ContainerInfo, InstanceInfo
from app.helpers.blueprint_helpers.aws.aws_instance_post import find_instance
from app.blueprints.aws.aws_container_blueprint import aws_container_bp
from app.constants.http_codes import (
    ACCEPTED,
)


def db_container_add(**kwargs):
    obj = ContainerInfo(
        container_id=kwargs["container_id"],
        instance_id=kwargs["instance_id"],
        user_id=kwargs["user_id"],
        status="ALLOCATED",
        ip=InstanceInfo.query.get(kwargs["instance_id"]).ip,
    )
    db.session.add(obj)
    db.session.commit()


# With pydantic, we can define our query/body datastructures with
# "one-time use" classes like this. They need to inherit from the pydantic
# BaseModel, but no other boilerplate after that. The type signatures are
# fully mypy compatible.
class ContainerAssignBody(BaseModel):
    region: str
    user_id: str
    dpi: int


# flask-pydantic ships a decorator called validate().
# When validate() is used, flask-pydantic can populate the arguments to your
# route function automatically.
#
# It's important to note that flask-pydantic actually uses the name of the
# parameter to decide which data to pass to it. So "body" must be used to
# access the request body, "query" must be used to access the query parameters.
# It must also have a type signature of a class thats inherited from BaseModel.
#
# That's it! No more boilerplate, the rest of the validation comes for free.
#
# If any of the required parameters to ContainerAssignBody are missing,
# the response will automatically 400 BAD REQUEST, and the response json
# will be something like the following:
#
# {
#    'validation_error': {
#        'query_params': [
#           {
#             'loc': ['dpi'],
#             'msg': ['field required'],
#             'type': ['value_error.missing'],
#           }
#        ]
#    }
# }
@aws_container_bp.route("/container_assign", methods=("POST",))
@validate()
def container_assign(body: ContainerAssignBody):
    region = body.get("region")
    user_id = body.get("username")
    dpi = body.get("dpi", 96)

    instance = find_instance(region)

    db_container_add(
        container_id=str(uuid.uuid4()),
        instance_id=instance.instance_id,
        user_id=user_id,
        dpi=dpi,
    )

    return jsonify({"ID": instance.id}), ACCEPTED
