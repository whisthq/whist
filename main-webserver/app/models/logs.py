from sqlalchemy.orm import relationship

from ._meta import db


class LoginHistory(db.Model):
    __tablename__ = "login_history"
    __table_args__ = {"extend_existing": True, "schema": "logs"}
    user_id = db.Column(db.ForeignKey("users.user_id"))
    user = relationship("User", back_populates="history")
    action = db.Column(db.String)
    timestamp = db.Column(db.Integer, primary_key=True)
