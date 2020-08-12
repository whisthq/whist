from app.models.public import *

class ReleaseGroup(Base):
    __tablename__ = 'release_groups'
    __table_args__ = {'extend_existing': True, 'schema': 'devops'}

    release_stage = Column(Integer, nullable=False)
    branch = Column(String(250), nullable=False, primary_key=True)