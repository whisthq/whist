from sqlalchemy.orm import relationship

from ._meta import db


class LoginHistory(db.Model):
    """
    Stores login/logout state for a given user

    Attributes:
        user_id (str): User ID, usually an email
        user (obj): reference to user in public.users with given user_id
        action (str): action of a user, logon or logoff
        timestamp (datetime): timestamp of action
    """

    __tablename__ = "login_history"
    __table_args__ = {"extend_existing": True, "schema": "logs"}
    user_id = db.Column(db.ForeignKey("users.user_id"))
    user = relationship("User", back_populates="history")
    action = db.Column(db.String)
    timestamp = db.Column(db.Integer, primary_key=True)
