from app.models.public import *
from app import db

class ProtocolLog(db.Model):
    __tablename__ = "protocol_logs"
    __table_args__ = {"extend_existing": True, "schema": "logs"}

    user_id = Column(String(250), index=True)
    server_logs = Column(String(250))
    connection_id = Column(String(250), nullable=False, primary_key=True)
    bookmarked = Column(Boolean, nullable=False)
    timestamp = Column(Integer)
    version = Column(String(250))
    client_logs = Column(String(250))

class MonitorLog(db.Model):
    __tablename__ = "monitor_logs"
    __table_args__ = {"extend_existing": True, "schema": "logs"}

    logons = Column(Integer)
    logoffs = Column(Integer)
    users_online = Column(Integer)
    eastus_available = Column(Integer)
    southcentralus_available = Column(Integer)
    northcentralus_available = Column(Integer)
    eastus_unavailable = Column(Integer)
    southcentralus_unavailable = Column(Integer)
    northcentralus_unavailable = Column(Integer)
    eastus_deallocated = Column(Integer)
    southcentralus_deallocated = Column(Integer)
    northcentralus_deallocated = Column(Integer)
    total_vms_deallocated = Column(Integer)
    timestamp = Column(Integer)

class LoginHistory(Base):
    __tablename__ = "login_history"
    __table_args__ = {"extend_existing": True, "schema": "logs"}
    user_id = Column(String)
    action = Column(String)
    timestamp = Column(Integer, primary_key=True)
