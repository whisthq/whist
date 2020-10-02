from sqlalchemy import Index
from sqlalchemy.orm import relationship
from sqlalchemy.schema import FetchedValue
from sqlalchemy.sql import expression, text

from app import db


class UserVM(db.Model):
    __tablename__ = "user_vms"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}

    vm_id = db.Column(db.String(250), primary_key=True, unique=True)
    ip = db.Column(db.String(250), nullable=False)
    location = db.Column(db.String(250), nullable=False)
    os = db.Column(db.String(250), nullable=False)
    state = db.Column(db.String(250), nullable=False)
    lock = db.Column(db.Boolean, nullable=False, server_default=expression.false())
    user_id = db.Column(db.ForeignKey("users.user_id"))
    disk_id = db.Column(db.ForeignKey("hardware.os_disks.disk_id"))
    temporary_lock = db.Column(db.Integer)


class UserContainer(db.Model):
    __tablename__ = "user_containers"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}
    container_id = db.Column(db.String(250), primary_key=True, unique=True)
    ip = db.Column(db.String(250), nullable=False)
    location = db.Column(db.String(250), nullable=False)
    os = db.Column(db.String(250), nullable=False)
    state = db.Column(db.String(250), nullable=False)
    lock = db.Column(db.Boolean, nullable=False, default=False)
    user_id = db.Column(db.ForeignKey("users.user_id"))
    user = relationship("User", back_populates="containers")
    port_32262 = db.Column(db.Integer, nullable=False)
    port_32263 = db.Column(db.Integer, nullable=False)
    port_32273 = db.Column(db.Integer, nullable=False)
    last_pinged = db.Column(db.Integer)
    cluster = db.Column(db.ForeignKey("hardware.cluster_info.cluster"))
    parent_cluster = relationship("ClusterInfo", back_populates="containers")
    using_stun = db.Column(db.Boolean, nullable=False, default=False)
    branch = db.Column(db.String(250), nullable=False, default="master")
    allow_autoupdate = db.Column(db.Boolean, nullable=False, default=True)
    temporary_lock = db.Column(db.Integer)
    secret_key = db.Column(db.LargeBinary(), server_default=FetchedValue())


class ClusterInfo(db.Model):
    __tablename__ = "cluster_info"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}
    cluster = db.Column(db.String(250), primary_key=True, unique=True)
    maxCPURemainingPerInstance = db.Column(db.Float, nullable=False, default=1024.0)
    maxMemoryRemainingPerInstance = db.Column(db.Float, nullable=False, default=2000.0)
    pendingTasksCount = db.Column(db.Integer, nullable=False, default=0)
    runningTasksCount = db.Column(db.Integer, nullable=False, default=0)
    registeredContainerInstancesCount = db.Column(db.Integer, nullable=False, default=0)
    minContainers = db.Column(db.Integer, nullable=False, default=0)
    maxContainers = db.Column(db.Integer, nullable=False, default=0)
    status = db.Column(db.String(250), nullable=False)
    containers = relationship(
        "UserContainer", back_populates="parent_cluster", lazy="dynamic", passive_deletes=True,
    )


class SortedClusters(db.Model):
    __tablename__ = "cluster_sorted"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}
    cluster = db.Column(db.String(250), primary_key=True, unique=True)
    maxCPURemainingPerContainer = db.Column(db.Float, nullable=False, default=1024.0)
    maxMemoryRemainingPerContainer = db.Column(db.Float, nullable=False, default=2000.0)
    pendingTasksCount = db.Column(db.Integer, nullable=False, default=0)
    runningTasksCount = db.Column(db.Integer, nullable=False, default=0)
    registeredContainerInstancesCount = db.Column(db.Integer, nullable=False, default=0)
    minContainers = db.Column(db.Integer, nullable=False, default=0)
    maxContainers = db.Column(db.Integer, nullable=False, default=0)
    status = db.Column(db.String(250), nullable=False)


class OSDisk(db.Model):
    __tablename__ = "os_disks"
    __table_args__ = {"schema": "hardware", "extend_existing": True}

    disk_id = db.Column(db.String(250), nullable=False, unique=True, primary_key=True)
    location = db.Column(db.String(250), nullable=False)
    os = db.Column(db.String(250), nullable=False)
    disk_size = db.Column(db.Integer, nullable=False)
    allow_autoupdate = db.Column(db.Boolean, nullable=False, server_default=expression.true())
    has_dedicated_vm = db.Column(db.Boolean, nullable=False, server_default=expression.false())
    version = db.Column(db.String(250))
    rsa_private_key = db.Column(db.String(250))
    using_stun = db.Column(db.Boolean, nullable=False, server_default=expression.false())
    ssh_password = db.Column(db.String(250))
    state = db.Column(db.String(250))
    user_id = db.Column(db.ForeignKey("users.user_id"))
    last_pinged = db.Column(db.Integer)
    branch = db.Column(
        db.String(250), nullable=False, server_default=text("'master'::character varying")
    )
    first_time = db.Column(db.Boolean, server_default=expression.true())


class SecondaryDisk(db.Model):
    __tablename__ = "secondary_disks"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}

    disk_id = db.Column(db.String(250), nullable=False, primary_key=True)
    os = db.Column(db.String(250), nullable=False)
    disk_size = db.Column(db.Integer, nullable=False)
    location = db.Column(db.String(250), nullable=False)
    user_id = db.Column(db.ForeignKey("users.user_id"))


class InstallCommand(db.Model):
    __tablename__ = "install_commands"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}

    install_command_id = db.Column(db.Integer, nullable=False, primary_key=True)
    windows_install_command = db.Column(db.String(250))
    linux_install_command = db.Column(db.String(250))
    app_name = db.Column(db.String(250))


class AppsToInstall(db.Model):
    __tablename__ = "apps_to_install"
    __table_args__ = (
        Index("fkIdx_115", "disk_id", "user_id"),
        {"schema": "hardware", "extend_existing": True},
    )

    disk_id = db.Column(db.String(250), primary_key=True, nullable=False)
    user_id = db.Column(db.String(250), primary_key=True, nullable=False)
    app_id = db.Column(db.String(250), nullable=False, index=True)


class SupportedAppImages(db.Model):
    __tablename__ = "supported_app_images"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}

    app_id = db.Column(db.String(250), nullable=False, unique=True, primary_key=True)
    logo_url = db.Column(db.String(250), nullable=False)
    task_definition = db.Column(db.String(250), nullable=False)
