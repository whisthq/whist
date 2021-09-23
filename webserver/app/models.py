"""SQLAlchemy models

In this module, we define Python models that represent tables in our PostgreSQL database. Each
model's attributes are automatically loaded from database column names when we call
``DeferredReflection.prepare()`` in ``app/factory.py``.

Note that SQLAlchemy is unable to reflect primary key constraints from views automatically. For
models representing SQL views (e.g. :class:`LingeringInstances`), the attribute representing the
primary key column must be defined explicitly.
"""

from flask_sqlalchemy import SQLAlchemy
from sqlalchemy.ext.declarative import DeferredReflection

db = SQLAlchemy(engine_options={"pool_pre_ping": True})


class InstanceInfo(DeferredReflection, db.Model):  # type: ignore[name-defined]
    """
    General information about our EC2 instances

    Attributes:
        instance_name (string): what is the instance called?
        cloud_provider_id (string): instance id from AWS console
        location (string): AWS region (i.e. us-east-1)
        instance_type (string): what hardware is the instance running on?
        ip (string): the instance's public IP
        nanocpus_remaining (float): CPU that isn't in use
        gpu_vram_remaining_kb (float): GPU that isn't in use
        memory_remaining_kb (float): RAM not in use
        mandelbox_capacity (int): how many mandelboxes can run at once?
        last_updated_utc_unix_ms (int): when did this instance last tell us it existed?
        creation_time_utc_unix_ms (int):  When was this instance created?
        aws_ami_id (str): what image is this machine based on?
        status (str): either PRE_CONNECTION, ACTIVE, HOST_SERVICE_UNRESPONSIVE or DRAINING
        commit_hash (str): what commit hash of our infrastructure is this machine running?"""

    __tablename__ = "instance_info"
    __table_args__ = {"schema": "hardware"}


class InstancesWithRoomForMandelboxes(DeferredReflection, db.Model):  # type: ignore[name-defined]
    """
    A map linking instance information to general mandelbox information
    i.e. 'how many mandelboxes are running on this instance',
    'how many can be', etc.

    Attributes:
        location (string): where is the instance?
        status (string): what's the instance's status?
        ami_id (string):  What image is the instance running?
        mandelbox_capacity (int): How many mandelboxes can the instance have?
        num_running_mandelboxes (int): and how many does it have?
    """

    __tablename__ = "instances_with_room_for_mandelboxes"
    __table_args__ = {"schema": "hardware"}

    #: str: A string that uniquely identifies the instance.
    instance_name = db.Column(db.String(), primary_key=True)


class LingeringInstances(DeferredReflection, db.Model):  # type: ignore[name-defined]
    """
    A view detailing which instances haven't updated recently so we can manually
    drain them.
    Specifically, it's all instances that are listed as either  active or preconnection,
    but that have not updated their status in the db for the specified period of time:
    2 min for running instances, 15 for preconnected instances


    Attributes:
        cloud_provider_id (string): What's it called on AWS?
        status (string):  What's it's most recent status??
    """

    __tablename__ = "lingering_instances"
    __table_args__ = {"schema": "hardware"}

    #: str: A string that uniquely identifies the instance.
    instance_name = db.Column(db.String(), primary_key=True)


class MandelboxInfo(DeferredReflection, db.Model):  # type: ignore[name-defined]
    """
    Information about individual mandelboxes, namely
    who's running them and where they're running.

    Attributes:
        mandelbox_id (int): which mandelbox is this?
        instance_name (string): the instance the mandelbox lives on.
        user_id (string): who requested it?
        status (string): is it running?
    """

    __tablename__ = "mandelbox_info"
    __table_args__ = {"schema": "hardware"}


class RegionToAmi(DeferredReflection, db.Model):  # type: ignore[name-defined]
    """
    This class represents the region_to_ami table in hardware
    it maps region names to the AMIs which should be used
    for instances in that region

    Attributes:
        region_name: The name of the region to which the AMI corresponds as a string.
        ami_id: A string representing the AMI ID of the latest AMI provisioned in the region
            corresponding to this row.
        client_commit_hash: A string representing the commit hash for the client.
        ami_active: A boolean that will be marked true if this AMI corresponds to
            an active versions of the client app'.
        protected_from_scale_down: A bool indicating whether the AMI is currently having an initial
            buffer spun up for it

    Constraints:
        Unique:
            _region_name_ami_id_unique_constraint: AMIs are expected to be unique per region
            and most likely global too.
            But didn't find any reference to back that up, so including a constraint.
    """

    __tablename__ = "region_to_ami"
    __table_args__ = {"schema": "hardware"}
