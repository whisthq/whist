import stripe

from sqlalchemy.orm import relationship
from sqlalchemy.sql import text
from sqlalchemy.sql.expression import func
from sqlalchemy.types import Text

from ._meta import db


class User(db.Model):
    """Fractal user account data.

    This SQLAlchemy model provides an interface to the public.users table of the database.

    Attributes:
        user_id (String): User ID, typically email
        name (String): Name of user (e.g. Mike)
        encrypted_config_token (String):  the user's app config token, stored encrypted
        token (String): The token that the user must use to initially . This token is generated
            when a user creates an account. It is sent in an email to the email address associated
            with the account. The email verification server endpoint compares the email
            verification token it receives to this token.
        encrypted_config_token (String):  the user's app config token, stored encrypted
        password (String): Hashed password
        stripe_customer_id (String): A pointer to a customer record on Fractal's Stripe account.
        reason_for_signup (String): How users heard about Fractal
        verified (Boolean): True/false email verified
        created_at (datetime): The timezone-aware date and time at which the user record was
            created.
    """

    __tablename__ = "users"

    user_id = db.Column(db.String(250), nullable=False, unique=True, primary_key=True)
    name = db.Column(db.String(250))
    token = db.Column(db.String(250))
    encrypted_config_token = db.Column(db.String(250))
    password = db.Column(db.String(250), nullable=False)
    stripe_customer_id = db.Column(db.String(250))
    reason_for_signup = db.Column(Text)
    verified = db.Column(db.Boolean, default=text("false"))
    created_at = db.Column(
        db.DateTime(timezone=True), nullable=False, server_default=func.current_timestamp()
    )

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

    @property
    def subscribed(self) -> bool:
        """Determine whether or not the user should receive service.

        Query the Stripe API to retrieve the user's subscription status.

        Returns:
            True iff the user has paid their monthly subscription fee or is benefitting from a free
            trial.
        """

        # The user won't have a stripe_customer_id until they have at least started a free trial.
        if self.stripe_customer_id is None:
            return False

        customer = stripe.Customer.retrieve(self.stripe_customer_id, expand=("subscriptions",))

        # The user should receive service as long as one of their Stripe subscriptions is active.
        for subscription in customer["subscriptions"]["data"]:
            if subscription["status"] in ("active", "trialing"):
                return True

        return False
