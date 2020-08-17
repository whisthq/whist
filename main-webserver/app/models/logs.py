from app.models.public import *

class ProtocolLog(Base):
    __tablename__ = 'protocol_logs'
    __table_args__ = {'extend_existing': True, 'schema': 'logs'}
    
    user_id = Column(String(250), index=True)
    server_logs = Column(String(250))
    connection_id = Column(String(250), nullable=False, primary_key=True)
    bookmarked = Column(Boolean, nullable=False)
    timestamp = Column(Integer)
    version = Column(String(250))
    client_logs = Column(String(250))