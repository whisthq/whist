from sqlalchemy.orm import relationship
from sqlalchemy.sql.expression import true
from sqlalchemy_utils.types.encrypted.encrypted_type import AesEngine, StringEncryptedType

from ._meta import db, secret_key


class UserContainer(db.Model):
    """User container data - information for spun up protocol containers

    This SQLAlchemy model provides an interface to the hardware.user_containers
    table of the database

    Attributes:
        container_id (string): container ID, taken from AWS
        ip (string): ip address of the ECS container instance (i.e. EC@ instance) on which the
            container is running
        location (string): AWS region of the container
        state (string): Container state, either [RUNNING_AVAILABLE, RUNNING_UNAVAILABLE,
            STOPPED, STOPPING, DELETING, CREATING, RESTARTING, STARTING]

            RUNNING_AVAILABLE: The protocol server is running, but no client has connected yet.
                We know when the protocol server is running once it sends its first ping back
                to the web server.
            RUNNING_UNAVAILABLE: The protocol server is running and a client is connected.

        user_id (string): User ID, typically email
        task_definition (string): foreign key to supported_app_images of streamed application
        app (SupportedAppImage): reference to hardware.supported_app_image obj of
            streamed application
        port_32262 (int): port number on the ECS container instance host that is forwraded to
            port 32262 on the container
        port_32263 (int): port number on the ECS container instance host that is forwraded to
            port 32263 on the container
        port_32273 (int): port number on the ECS container instance host that is forwraded to
            port 32273 on the container
        last_pinged (int): timestamp representing when the container was last pinged
        cluster (string): foreign key for the cluster the container is hosted on
        parent_cluster (ClusterInfo): reference to hardware.cluster_info object of the parent
        dpi (int): pixel density of the stream
        secret_key (string): 16-byte AES key used to encrypt communication between protocol
            server and client


    """

    __tablename__ = "user_containers"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}
    container_id = db.Column(db.String(250), primary_key=True, unique=True)
    ip = db.Column(db.String(250), nullable=False)
    location = db.Column(db.String(250), nullable=False)
    state = db.Column(db.String(250), nullable=False)
    user_id = db.Column(db.String(), nullable=True)
    task_definition = db.Column(db.ForeignKey("hardware.supported_app_images.task_definition"))
    task_version = db.Column(db.Integer, nullable=True)
    app = relationship("SupportedAppImages", back_populates="containers")
    port_32262 = db.Column(db.Integer, nullable=False)
    port_32263 = db.Column(db.Integer, nullable=False)
    port_32273 = db.Column(db.Integer, nullable=False)
    last_pinged = db.Column(db.Integer)
    cluster = db.Column(db.ForeignKey("hardware.cluster_info.cluster"))
    parent_cluster = relationship("ClusterInfo", back_populates="containers")
    dpi = db.Column(db.Integer)
    secret_key = db.Column(
        StringEncryptedType(db.String, secret_key, AesEngine, "pkcs5"), nullable=False
    )


class ClusterInfo(db.Model):
    """Stores data for ECS clusters spun up by the webserver. Containers information such as
       total task count, number of running instances, and container status.

    Attributes:
        cluster (string): cluster id from AWS console
        location (string): AWS region (i.e. us-east-1)
        maxCPURemainingPerInstance (float): maximum available CPU that has not
            been allocated to tasks
        maxMemoryRemainingPerInstance (float): maximum avilable RAM that has not
            been allocated to tasks
        pendingTaskscount (int): number of tasks in the cluster that are in the PENDING state
        runningTasksCount (int): number of tasks in the cluster that are in the RUNNING state
        registeredContainerInstancesCount (int): The number of container instances registered
            into the cluster. This includes container instances in both ACTIVE and DRAINING status.
        minContainers (int): The minimum size of the Auto Scaling group.
        maxContainers (int): The maximum size of the Auto Scaling Group
        status: (string): Status of the cluster, either [ACTIVE, PROVISIONING,
            DEPROVISINING, FAILED, INACTIVE]
        containers (UserContainer): reference to hardware.user_containers of containers
            within given cluster

    """

    __tablename__ = "cluster_info"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}
    cluster = db.Column(db.String(250), primary_key=True, unique=True)
    location = db.Column(db.String(250), nullable=False, default="N/a")
    maxCPURemainingPerInstance = db.Column(db.Float, nullable=False, default=1024.0)
    maxMemoryRemainingPerInstance = db.Column(db.Float, nullable=False, default=2000.0)
    pendingTasksCount = db.Column(db.Integer, nullable=False, default=0)
    runningTasksCount = db.Column(db.Integer, nullable=False, default=0)
    registeredContainerInstancesCount = db.Column(db.Integer, nullable=False, default=0)
    minContainers = db.Column(db.Integer, nullable=False, default=0)
    maxContainers = db.Column(db.Integer, nullable=False, default=0)
    status = db.Column(db.String(250), nullable=False)
    containers = relationship(
        "UserContainer",
        back_populates="parent_cluster",
        lazy="dynamic",
        passive_deletes=True,
    )


class SortedClusters(db.Model):
    """Represents the cluster_sorted view, which contains data from cluster_info.

    The rows in cluster_sorted are sorted by cluster_info.registeredContainerInstancesCount.

    Attributes:
        See app.models.hardware.ClusterInfo.
    """

    __tablename__ = "cluster_sorted"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}
    cluster = db.Column(db.String(250), primary_key=True, unique=True)
    location = db.Column(db.String(250), nullable=False, default="N/a")
    maxCPURemainingPerInstance = db.Column(db.Float, nullable=False, default=1024.0)
    maxMemoryRemainingPerInstance = db.Column(db.Float, nullable=False, default=2000.0)
    pendingTasksCount = db.Column(db.Integer, nullable=False, default=0)
    runningTasksCount = db.Column(db.Integer, nullable=False, default=0)
    registeredContainerInstancesCount = db.Column(db.Integer, nullable=False, default=0)
    minContainers = db.Column(db.Integer, nullable=False, default=0)
    maxContainers = db.Column(db.Integer, nullable=False, default=0)
    status = db.Column(db.String(250), nullable=False)
    containers = relationship(
        "UserContainer",
        lazy="dynamic",
        passive_deletes=True,
        primaryjoin="foreign(UserContainer.cluster) == SortedClusters.cluster",
    )


class InstanceInfo(db.Model):
    """
    General information about our EC2 instances

    Attributes:
        instance_id (string): instance id from AWS console
        location (string): AWS region (i.e. us-east-1)
        instance_type (string): what hardware is the instance running on?
        auth_token (string): what token does this instance use to auth with the webserver?
        ip (string): the instance's public IP
        CPURemainingPerInstance (float): CPU that isn't in use
        GPURemainingPerInstance (float): GPU that isn't in use
        memoryRemainingPerInstance (float): RAM not in use
        maxContainers (int): how many containers can run at once?
        last_pinged (int): when did this instance last tell us it existed?
        created_at (int):  When was this instance created?
        ami_id (str): what image is this machine based on?
        is_active (boolean):  is this instance active or inactive?"""

    __tablename__ = "instance_info"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}
    instance_id = db.Column(db.String(250), primary_key=True, unique=True)
    location = db.Column(db.String(250), nullable=False)
    instance_type = db.Column(db.String(250), nullable=False)
    auth_token = db.Column(db.String(250))
    ip = db.Column(db.String(250))
    CPURemainingInInstance = db.Column(db.Float, nullable=False, server_default="1024.0")
    GPURemainingInInstance = db.Column(db.Float, nullable=False, server_default="1024.0")
    memoryRemainingInInstanceInMb = db.Column(db.Float, nullable=False, server_default="2000.0")
    maxContainers = db.Column(db.Integer, nullable=False, default=0)
    last_pinged = db.Column(db.Integer)
    created_at = db.Column(db.Integer)
    ami_id = db.Column(db.String(250), nullable=False)
    status = db.Column(db.String(250), nullable=False)


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
    instance_id = db.Column(db.String(250), primary_key=True, unique=True)
    location = db.Column(db.String(250), nullable=False)
    ami_id = db.Column(db.String(250), nullable=False)


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
    instance_id = db.Column(db.String(250), primary_key=True, unique=True)
    location = db.Column(db.String(250), nullable=False)
    ami_id = db.Column(db.String(250), nullable=False)
    max_containers = db.Column(db.Integer)
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
    container_id = db.Column(db.String(250), primary_key=True)
    instance_id = db.Column(db.String(250), nullable=False)
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
        allowed: A boolean indicating whether or not users are allowed to deploy tasks in the
            region corresponding to this row.
    """

    __tablename__ = "region_to_ami"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}
    region_name = db.Column(db.String(250), nullable=False, unique=True, primary_key=True)
    ami_id = db.Column(db.String(250), nullable=False)
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
    containers = relationship(
        "UserContainer",
        back_populates="app",
        lazy="dynamic",
        passive_deletes=True,
    )


class UserContainerState(db.Model):
    """Stores basic state information which users can query or subscribe to, to
    find out whether their container/app is ready, pending etc. Check
    container_state_values (constants) for he list of possible states and
    container_state.py in helpers/blueprint_helpers for the methods which you can use
    for this table.

    Args:
        db (SLQAlchemy db): Implements db methods to communicate with the physical infra.
    """

    __tablename__ = "user_app_state"  # may want to change going forward
    __table_args__ = {"extend_existing": True, "schema": "hardware"}

    user_id = db.Column(db.String(), primary_key=True)
    state = db.Column(db.String(250), nullable=False)
    ip = db.Column(db.String(250))
    client_app_auth_secret = db.Column(db.String(250))
    port = db.Column(db.Integer)
    task_id = db.Column(db.String(250), nullable=False)
