from ._meta import db


class ReleaseGroup(db.Model):
    __tablename__ = "release_groups"
    __table_args__ = {"extend_existing": True, "schema": "devops"}

    release_stage = db.Column(db.Integer, nullable=False)
    branch = db.Column(db.String(250), nullable=False, primary_key=True)
