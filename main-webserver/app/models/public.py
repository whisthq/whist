from sqlalchemy.orm import relationship
from sqlalchemy.sql import text
from sqlalchemy.types import Text

from ._meta import db


class User(db.Model):
    __tablename__ = "users"

    user_id = db.Column(db.String(250), nullable=False, unique=True, primary_key=True)
    name = db.Column(db.String(250))
    token = db.Column(db.String(250))
    password = db.Column(db.String(250), nullable=False)
    release_stage = db.Column(db.Integer, nullable=False, default=text("50"))
    stripe_customer_id = db.Column(db.String(250))
    created_timestamp = db.Column(db.Integer)
    reason_for_signup = db.Column(Text)
    referral_code = db.Column(db.String(250))
    credits_outstanding = db.Column(db.Integer, default=text("0"))
    using_google_login = db.Column(db.Boolean, default=text("false"))
    verified = db.Column(db.Boolean, default=text("false"))
    using_facebook_login = db.Column(db.Boolean, default=text("false"))
    can_login = db.Column(db.Boolean, default=text("false"))
    card_brand = db.Column(db.String(250))
    card_last_four = db.Column(db.String(250))
    postal_code = db.Column(db.String(250))

    # Setting passive_deletes causes SQLAlchemy to defer to the database to
    # handle, e.g., cascade deletes. Setting the value to "all" may work as
    # well. See
    # https://docs.sqlalchemy.org/en/13/orm/relationship_api.html#sqlalchemy.orm.relationship.params.passive_deletes
    containers = relationship(
        "UserContainer", back_populates="user", lazy="dynamic", passive_deletes=True
    )
    history = relationship(
        "LoginHistory", back_populates="user", lazy="dynamic", passive_deletes=True
    )
    protocol_logs = relationship(
        "ProtocolLog", back_populates="user", lazy="dynamic", passive_deletes=True
    )
