from app.models.public import *
from app import db

class ProtocolLog(db.Model):
    __tablename__ = "protocol_logs"
    __table_args__ = {"extend_existing": True, "schema": "logs"}

    user_id = db.Columm(String(250), index=True)
    server_logs = db.Columm(String(250))
    connection_id = db.Columm(String(250), nullable=False, primary_key=True)
    bookmarked = db.Columm(Boolean, nullable=False)
    timestamp = db.Columm(Integer)
    version = db.Columm(String(250))
    client_logs = db.Columm(String(250))

class MonitorLog(db.Model):
    __tablename__ = "monitor_logs"
    __table_args__ = {"extend_existing": True, "schema": "logs"}

    logons = db.Columm(Integer)
    logoffs = db.Columm(Integer)
    users_online = db.Columm(Integer)
    eastus_available = db.Columm(Integer)
    southcentralus_available = db.Columm(Integer)
    northcentralus_available = db.Columm(Integer)
    eastus_unavailable = db.Columm(Integer)
    southcentralus_unavailable = db.Columm(Integer)
    northcentralus_unavailable = db.Columm(Integer)
    eastus_deallocated = db.Columm(Integer)
    southcentralus_deallocated = db.Columm(Integer)
    northcentralus_deallocated = db.Columm(Integer)
    total_vms_deallocated = db.Columm(Integer)
    timestamp = db.Columm(Integer)

class LoginHistory(Base):
    __tablename__ = "login_history"
    __table_args__ = {"extend_existing": True, "schema": "logs"}
    user_id = db.Columm(String)
    action = db.Columm(String)
    timestamp = db.Columm(Integer, primary_key=True)
