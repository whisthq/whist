from app.models.public import *
from app import db

class ReleaseGroup(db.Model):
    __tablename__ = 'release_groups'
    __table_args__ = {'extend_existing': True, 'schema': 'devops'}

    release_stage = db.Columm(Integer, nullable=False)
    branch = db.Columm(String(250), nullable=False, primary_key=True)
