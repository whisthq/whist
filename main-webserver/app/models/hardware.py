from sqlalchemy import UniqueConstraint
from sqlalchemy.sql.expression import true
from ._meta import db


class InstanceInfo(db.Model):
    """
    General information about our EC2 instances

    Attributes:
        instance_name (string): what is the instance called?
        cloud_provider_id (string): instance id from AWS console
        location (string): AWS region (i.e. us-east-1)
        instance_type (string): what hardware is the instance running on?
        auth_token (string): what token does this instance use to auth with the webserver?
        ip (string): the instance's public IP
        nanocpus_remaining (float): CPU that isn't in use
        gpu_vram_remaining_kb (float): GPU that isn't in use
        memory_remaining_kb (float): RAM not in use
        container_capacity (int): how many containers can run at once?
        last_updated_utc_unix_ms (int): when did this instance last tell us it existed?
        creation_time_utc_unix_ms (int):  When was this instance created?
        aws_ami_id (str): what image is this machine based on?
        status (str): either PRE-CONNECTION, ACTIVE, or DRAINING
        commit_hash (str): what commit hash of our infrastructure is this machine running?"""

    __tablename__ = "instance_info"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}
    instance_name = db.Column(db.String(250), primary_key=True, unique=True)
    cloud_provider_id = db.Column(db.String(250), nullable=False)
    location = db.Column(db.String(250), nullable=False)
    creation_time_utc_unix_ms = db.Column(db.Integer, nullable=False)
    aws_instance_type = db.Column(db.String(250), nullable=False)
    auth_token = db.Column(db.String(250), nullable=False)
    ip = db.Column(db.String(250), nullable=False)
    nanocpus_remaining = db.Column(db.Integer, nullable=False, server_default="1024")
    gpu_vram_remaining_kb = db.Column(db.Integer, nullable=False, server_default="1024")
    memory_remaining_kb = db.Column(db.Integer, nullable=False, server_default="2000")
    container_capacity = db.Column(db.Integer, nullable=False, default=0)
    last_updated_utc_unix_ms = db.Column(db.Integer, nullable=False, server_default="-1")
    aws_ami_id = db.Column(db.String(250), nullable=False)
    status = db.Column(db.String(250), nullable=False)
    commit_hash = db.Column(db.String(250), nullable=False)


class InstanceSorted(db.Model):
    """
    A sorted list of instance IDs and info, for selecting where
    we deploy incoming tasks to.

    Attributes:
        instance_id (string): instance id from AWS console
        location (string): where is the instance?
        ami_id (string): What image is the instance running?
    """

    __tablename__ = "instance_allocation"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}
    instance_name = db.Column(db.String(250), primary_key=True, unique=True)
    location = db.Column(db.String(250), nullable=False)
    aws_ami_id = db.Column(db.String(250), nullable=False)


class InstancesWithRoomForContainers(db.Model):
    """
    A map linking instance information to general container information
    i.e. 'how many containers are running on this instance',
    'how many can be', etc.

    Attributes:
        instance_id (string): instance id from AWS console
        location (string): where is the instance?
        ami_id (string):  What image is the instance running?
        max_containers (int): How many containers can the instance have?
        num_running_containers (int): and how many does it have?
    """

    __tablename__ = "instance_sorted"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}
    instance_name = db.Column(db.String(250), primary_key=True, unique=True)
    location = db.Column(db.String(250), nullable=False)
    aws_ami_id = db.Column(db.String(250), nullable=False)
    container_capacity = db.Column(db.Integer)
    num_running_containers = db.Column(db.Integer)


class ContainerInfo(db.Model):
    """
    Information about individual containers, namely
    who's running them and where they're running.

    Attributes:
        container_id (int):  which container is this?
        instance_id (string): which instance is it on?
        user_id (string): who's running it?
        status (string): is it running?
    """

    __tablename__ = "container_info"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}
    creation_time_utc_unix_ms = db.Column(db.Integer, nullable=False)
    container_id = db.Column(db.String(250), primary_key=True)
    instance_name = db.Column(db.String(250), nullable=False)
    user_id = db.Column(db.String(250), nullable=False)
    status = db.Column(db.String(250), nullable=False)


class RegionToAmi(db.Model):
    """
    This class represents the region_to_ami table in hardware
    it maps region names to the AMIs which should be used
    for clusters in that region

    Attributes:
        region_name: The name of the region to which the AMI corresponds as a string.
        ami_id: A string representing the AMI ID of the latest AMI provisioned in the region
            corresponding to this row.
        client_commit_hash: A string representing the commit hash for the client.
        enabled: A boolean that will be marked true if this AMI corresponds to 
            an active versions of the client app'.
        allowed: A boolean indicating whether or not users are allowed to deploy tasks in the
            region corresponding to this row.

    Constraints:
        Unique:
            _region_name_ami_id_unique_constraint: AMIs are expected to be unique per region  
            and most likely global too. 
            But didn't find any reference to back that up, so including a constraint.
    """

    __tablename__ = "region_to_ami"
    __table_args__ = (
        UniqueConstraint("region_name", "ami_id", name="_region_name_ami_id_unique_constraint"),
        {"extend_existing": True, "schema": "hardware"},
    )
    region_name = db.Column(db.String(250), nullable=False, primary_key=True)
    ami_id = db.Column(db.String(250), nullable=False)
    client_commit_hash = db.Column(db.String(40), nullable=False, primary_key=True)
    enabled = db.Column(db.Boolean, nullable=False, server_default=true())
    allowed = db.Column(db.Boolean, nullable=False, server_default=true())


class SupportedAppImages(db.Model):
    __tablename__ = "supported_app_images"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}

    app_id = db.Column(db.String(250), nullable=False, unique=True, primary_key=True)
    logo_url = db.Column(db.String(250), nullable=False)
    task_definition = db.Column(db.String(250), nullable=False)
    task_version = db.Column(db.Integer, nullable=True)
    category = db.Column(db.String(250))
    description = db.Column(db.String(250))
    long_description = db.Column(db.String(250))
    url = db.Column(db.String(250))
    tos = db.Column(db.String(250))
    active = db.Column(db.Boolean, nullable=False, default=False)
    # The coefficient delineating what fraction of live users we should have
    # as a prewarmed buffer
    preboot_number = db.Column(db.Float, nullable=False)
