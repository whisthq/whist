from sqlalchemy import Index
from sqlalchemy.orm import relationship
from sqlalchemy.sql import expression, text

from ._meta import db


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
    secret_key = db.Column(db.String(32), nullable=False)


class ClusterInfo(db.Model):
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
    category = db.Column(db.String(250))
    description = db.Column(db.String(250))
    long_description = db.Column(db.String(250))
    url = db.Column(db.String(250))
    tos = db.Column(db.String(250))
    active = db.Column(db.Boolean, nullable=False, default=False)


class Banners(db.Model):
    __tablename__ = "banners"
    __table_args__ = {"extend_existing": True, "schema": "hardware"}

    heading = db.Column(db.String(250), nullable=False, unique=True, primary_key=True)
    subheading = db.Column(db.String(250))
    category = db.Column(db.String(250))
    background = db.Column(db.String(250))
    url = db.Column(db.String(250))
