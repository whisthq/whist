from typing import Any, Dict, Optional, Union

import stripe

from pyzipcode import ZipCodeDatabase

from app.helpers.utils.general.time import date_to_unix, get_today

from app.models import db, User
from app.serializers.public import UserSchema


class NonexistentUser(Exception):
    """This exception is raised when the user (customer) does not exist on a charge or
    similar transaction."""


class NonexistentStripeCustomer(Exception):
    """This exception is raised when the user is not
    already a stripe customer."""


class RegionNotSupported(Exception):
    """We only suppor the NA (North America) region currently. In the past we only
    supported the US, though in the future we'd like to go global. This exception is
    raised when transactions are made for customers outside supported regions."""


class InvalidStripeToken(Exception):
    """This exception is raised when a customer passes an invalid token to an endpoint.
    For example, if they are trying to charge and pass in a token that stripe cannot parse
    we will raise this issue."""


class InvalidOperation(Exception):
    """This is raised when you try to delete your subscription if you are not subscribed
    or you try to create a subscription already being subscribed without modifying."""


class StripeClient:
    # api key must be the stripe secret key or
    # a restrited "secret" key as in the stripe dashboard
    # go to the keys section on the bottom left and search for "limitted access"
    def __init__(self, api_key: str):
        """Initialize a reusable StripeClient object that can be used to execute various
        actions for a client by our stripe endpoints.

        Args:
            api_key (str): A stripe private key, or a restricted access key
        """
        # this is not saved in our class for security reasons
        # also we do not need it
        stripe.api_key = api_key

        self.zipcode_db = ZipCodeDatabase()

        self.user_schema = UserSchema()

    @staticmethod
    def refresh_key(api_key: str) -> None:
        """Reset the stripe api key.

        Args:
            api_key (str): A stripe api key that we want to set as
                the new value of the api key. We may want to do this
                if we are executing various commands or reusing an object
                in different endpoints, and don't want someone to be able
                to use one key to do various things (minimizing damage).
        """
        stripe.api_key = api_key

    @staticmethod
    def validate_customer_id(stripe_customer_id: str, user: Optional[User] = None) -> bool:
        """Confirms that a stripe customer id that we store is valid and is not
        deleted, for example. The use case is when we manually delete people from
        the stripe dashboard, we don't want to break the code (the db isn't updated immediately).

        Then it will delete a stripe_customer_id from the database if it does not exist

        Args:
            stripe_customer_id (str): The customer id string of the stripe customer.
            user (User, optional): The user for whom we might want to update the db.

        Returns:
            False if the stripe_customer_id was not valid

        Raises:
            stripe.error.InvalidRequestError if customer is not retrievable
        """
        if not stripe_customer_id:
            return True

        try:
            customer = stripe.Customer.retrieve(stripe_customer_id)
            if "deleted" in customer and customer["deleted"]:
                raise RuntimeError("Customer deleted.")
        except Exception as e:
            if user:
                user.stripe_customer_id = None
                db.session.commit()
                return False
            else:
                raise e
        return True

    def get_customer_info(self, email: str) -> Dict[str, Union[str, bool, Dict[str, Any]]]:
        """Retrieves the information for a user regarding stripe. Returns in the following format:
        {
            "subscription": a subscription object per the stripe api (json format),
            "cards": cards object per stripe api (also json format),
            "account_locked": whether your account is account_locked,
            "customer": customer as a json - the schema dump,
        } or None if the customer does not exist

        This will throw an exception of unknown type if the subscription fetching fails.

        Args:
            email (str): The email for the user.
        """

        # get the user if they are valid/exist
        user = User.query.get(email)
        if not user:
            raise NonexistentUser

        customer = self.user_schema.dump(user)
        stripe_customer_id = customer["stripe_customer_id"]

        # return a valid object if they exist else None
        if stripe_customer_id and self.validate_customer_id(stripe_customer_id, user):

            subscription = stripe.Subscription.list(customer=stripe_customer_id)

            if subscription and subscription["data"] and len(subscription["data"]) > 0:
                subscription = subscription["data"][0]
                account_locked = subscription["trial_end"] < date_to_unix(get_today())
            else:
                subscription = None
                account_locked = False

            return {
                "subscription": subscription,
                "account_locked": account_locked,
                "customer": customer,
            }
        else:
            return None
