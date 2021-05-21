import uuid
from flask import Blueprint
from flask.json import jsonify
from pydantic import BaseModel
from flask_pydantic import validate
from sqlalchemy.orm import relationship
from app.models import db
from app.helpers.utils.general.sql_commands import fractal_sql_commit
from app.constants.http_codes import (
    ACCEPTED,
)


from app.factory import create_app
from app.models import UserContainer

app = create_app()

with app.app_context():
    base_container = (
        UserContainer.query.filter(
            UserContainer.task_definition == "fractal-dev-browsers-chrome",
            UserContainer.location == "us-east-1",
            UserContainer.user_id is not None,
        )
        .filter(UserContainer.cluster.notlike("%test%"))
        .with_for_update()
        .limit(1)
        .first()
    )

    print(base_container)

aws_container_bp = Blueprint("aws_container_bp", __name__)


class InstanceInfo(db.Model):
    __tablename__ = "instance_info"
    instance_id = db.Column(db.String(250), primary_key=True, unique=True)
    instance_type = db.Column(db.String(250), nullable=False)
    ami_id = db.Column(db.String(250), nullable=False)
    region = db.Column(db.String(250), nullable=False)
    ip = db.Column(db.String(250), nullable=False)
    containers = relationship("ContainerInfo")
    containerLimit = db.Column(db.Integer)
    CPURemainingInInstance = db.Column(
        db.Float, nullable=False, default=1024.0
    )
    GPURemainingInInstance = db.Column(
        db.Float, nullable=False, default=1024.0
    )
    memoryRemainingInInstanceInMb = db.Column(
        db.Float, nullable=False, default=2000.0
    )


class ContainerInfo(db.Model):
    __tablename__ = "container_info"
    container_id = db.Column(db.String(250), primary_key=True, unique=True)
    instance_id = db.Column(
        db.ForeignKey("instance_info.instance_id"), nullable=False
    )
    user_id = db.Column(db.ForeignKey("users.user_id"), nullable=False)
    status = db.Column(db.String(250), nullable=False)
    dpi = db.Column(db.Integer, nullable=False)
    ip = db.Column(db.String(250), nullable=False)


def db_container_add(**kwargs):
    obj = ContainerInfo(
        container_id=kwargs["container_id"],
        instance_id=kwargs["instance_id"],
        user_id=kwargs["user_id"],
        status="ALLOCATED",
        ip=InstanceInfo.query.get(kwargs["instance_id"]).ip,
    )
    fractal_sql_commit(db, lambda db, x: db.session.add(x), obj)


def choose_instance(region):
    instances = db.instances.filter_by(region=region)
    instances.sort(reverse=True, key=lambda x: x.containerLimit - x.containers)
    return instances[0]


class ContainerAssignBody(BaseModel):
    region: str
    user_id: str
    dpi: int


@aws_container_bp.route("/container_assign", methods=("POST",))
@validate()
def container_assign(body: ContainerAssignBody):
    region = body.get("region")
    user_id = body.get("username")
    dpi = body.get("dpi", 96)

    instance = choose_instance(region)

    db_container_add(
        container_id=str(uuid.uuid4()),
        instance_id=instance.instance_id,
        user_id=user_id,
        dpi=dpi,
    )

    return jsonify({"ID": instance.id}), ACCEPTED
