from sqlalchemy.orm import relationship
from sqlalchemy.sql import expression, text
from sqlalchemy.types import Text

from ._meta import db


class User(db.Model):
    """Fractal user account data.

    This SQLAlchemy model provides an interface to the public.users table of the database.

    Attributes:
        user_id (String): User ID, typically email
        name (String): Name of user (e.g. Mike)
        token (String): The token that the user must use to initially . This token is generated
            when a user creates an account. It is sent in an email to the email address associated
            with the account. The email verification server endpoint compares the email
            verification token it receives to this token.
        password (String): Hashed password
        stripe_customer_id (String): A pointer to a customer record on Fractal's Stripe account.
        subscribed (Boolean): A boolean indicating whether or not the user should receive service.
            True iff the User's stripe subscription is "active" or "trialing".
        reason_for_signup (String): How users heard about Fractal
        verified (Boolean): True/false email verified
    """

    __tablename__ = "users"

    user_id = db.Column(db.String(250), nullable=False, unique=True, primary_key=True)
    name = db.Column(db.String(250))
    token = db.Column(db.String(250))
    password = db.Column(db.String(250), nullable=False)
    stripe_customer_id = db.Column(db.String(250))
    subscribed = db.Column(db.Boolean, server_default=expression.true())
    created_timestamp = db.Column(db.Integer)
    reason_for_signup = db.Column(Text)
    verified = db.Column(db.Boolean, default=text("false"))

    # Setting passive_deletes causes SQLAlchemy to defer to the database to
    # handle, e.g., cascade deletes. Setting the value to "all" may work as
    # well. See
    # https://docs.sqlalchemy.org/en/13/orm/relationship_api.html#sqlalchemy.orm.relationship.params.passive_deletes
    containers = relationship(
        "UserContainer", back_populates="user", lazy="dynamic", passive_deletes=True
    )
    credentials = relationship("Credential", backref="user")
    history = relationship(
        "LoginHistory", back_populates="user", lazy="dynamic", passive_deletes=True
    )
