"""SQLAlchemy models

This module defines Python models that represent tables in our PostgreSQL database. Each model's
attributes are automatically loaded from database column names when we call
``DeferredReflection.prepare()`` in ``app/utils/flask/factory.py``.
"""

from enum import auto, Enum

from flask_sqlalchemy import SQLAlchemy
from sqlalchemy.ext.declarative import DeferredReflection
from sqlalchemy.orm import relationship

db = SQLAlchemy(engine_options={"pool_pre_ping": True})


class NoValue(Enum):
    # https://docs.python.org/3/library/enum.html#omitting-values

    def __repr__(self) -> str:
        # pylint: disable=consider-using-f-string
        return "<%s.%s>" % (self.__class__.__name__, self.name)


class CloudProvider(NoValue):
    """The cloud provider that runs the instance."""

    AWS = auto()


class MandelboxHostState(NoValue):
    PRE_CONNECTION = auto()
    ACTIVE = auto()
    DRAINING = auto()


class MandelboxState(NoValue):
    ALLOCATED = auto()
    CONNECTING = auto()
    RUNNING = auto()
    DYING = auto()


class WhistApplication(NoValue):
    """The application that is being streamed by a Mandelbox."""

    CHROME = auto()


class Image(DeferredReflection, db.Model):  # type: ignore[misc,name-defined]
    """A virtual machine image that can be used to launch Mandelbox hosts.

    Attributes:
        provider: The cloud provider that manages the image.
        region: The region in which the image can be used to launch instances.
        image_id: The unique string that its cloud provider uses to identify the image.
        client_sha: The source commit hash from which the image was built.
        updated_at: The time at which the row was last updated.
    """

    __tablename__ = "images"
    __table_args__ = {"schema": "whist"}

    provider = db.Column(db.Enum(CloudProvider), primary_key=True)


class Instance(DeferredReflection, db.Model):  # type: ignore[misc,name-defined]
    """A virtual machine that can host Mandelboxes.

    Attributes:
        id: The unique string that the its cloud provider uses to identify the instance.
        provider: The cloud provider that runs the instance.
        region: The region in which the instance is running.
        image_id: A unique string that the cloud provider uses to identify the image from which the
            instance was launched.
        client_sha: The source commit hash from which the image from which the instance was
            launched was built.
        ip_addr: The instance's public IPv4 address.
        instance_type: A cloud-provider-specific string that its cloud provider uses to identify
            the instance's type.
        remaining_capacity: The number of additional Mandelboxes that the instance can accomodate.
        status: The connection status of the host service running on the instance.
        created_at: The time at which the instance was launched.
        updated_at: The time at which the row was last updated.
        mandelboxes: The Mandelboxes that are running on this instance.
    """

    __tablename__ = "instances"
    __table_args__ = {"schema": "whist"}

    provider = db.Column(db.Enum(CloudProvider), nullable=False)
    status = db.Column(db.Enum(MandelboxHostState), nullable=False)
    mandelboxes = relationship("Mandelbox", back_populates="host")


class Mandelbox(DeferredReflection, db.Model):  # type: ignore[misc,name-defined]
    """A container from which a Whist application is being streamed.

    Attributes:
        id: A unique string that identifies the Mandelbox's underlying Docker container.
        app: The application that is being streamed from within the Mandelbox.
        instance_id: A foreign key to the instances table. The primary key of the instance on which
            the Mandelbox is running.
        user_id: A string that identifies the user who launched the Mandelbox.
        session_id: ???
        status: The lifecycle state of the Mandelbox.
        created_at: The time at which the Mandelbox was launched.
    """

    __tablename__ = "mandelboxes"
    __table_args__ = {"schema": "whist"}

    app = db.Column(db.Enum(WhistApplication), nullable=False)
    status = db.Column(db.Enum(MandelboxState), nullable=False)
    host = relationship("Instance", back_populates="mandelboxes")
