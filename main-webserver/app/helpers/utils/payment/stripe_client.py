import time
from datetime import timedelta
from typing import Dict, Optional

import stripe

from pyzipcode import ZipCodeDatabase

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

    @staticmethod
    def get_customer_info(customer_id: str) -> Dict[str, Optional[time.struct_time]]:
        """Retrieves the information for a user regarding stripe. Returns in the following format:
        {
            "trial_end": returns the end of the most recent free trial
            "access_end": returns the end of the current subscription period
        } or None if the customer does not exist

        This will throw an exception of unknown type if the subscription fetching fails.

        Args:
            customer_id (str): The user's stripe customer ID.
        """
        res_dict: Dict[str, Optional[time.struct_time]] = {
            "customer": None,
            "trial_end": None,
            "access_end": None,
        }
        client_info = stripe.Customer.retrieve(customer_id, expand=["subscriptions"])
        subscriptions = client_info["subscriptions"]
        if len(subscriptions["data"]) > 0:
            curr_subscription = subscriptions["data"][0]
            trial_end: time.struct_time = time.localtime(curr_subscription["trial_end"])
            curr_end: time.struct_time = time.localtime(curr_subscription["current_period_end"])
            res_dict["trial_end"] = trial_end
            res_dict["access_end"] = curr_end
        return res_dict

    def is_paying(self, customer_id: str) -> bool:
        """
        Returns whether a customer is currently paying for fractal
        Args:
            customer_id: the stripe customer ID

        Returns: True IFF the customer is on a paid subscription

        """
        access_val: Optional[time.struct_time] = self.get_customer_info(customer_id)["access_end"]
        return access_val is not None and access_val > time.localtime(time.time())

    def is_trialed(self, customer_id: str) -> bool:
        """
        Returns whether a customer is currently trialed for fractal
        Args:
            customer_id: the stripe customer ID

        Returns: True IFF the customer is on a free trial

        """
        trial_val: Optional[time.struct_time] = self.get_customer_info(customer_id)["trial_end"]
        return trial_val is not None and trial_val > time.localtime(time.time())

    def create_checkout_session(
        self, success_url: str, cancel_url: str, customer_id: str, price_id: str
    ) -> str:
        """
        Returns checkout session id from Stripe client

        Args:
            customer_id (str): the stripe id of the user
            price_id (str): the price id of the product (subscription)
            success_url (str): url to redirect to upon completion success
            cancel_url (str): url to redirect to upon cancelation

        Returns:
            json, int: Json containing session id and status code
        """
        try:
            checkout_session = stripe.checkout.Session.create(
                success_url=success_url,
                cancel_url=cancel_url,
                customer=customer_id,
                payment_method_types=["card"],
                mode="subscription",
                line_items=[{"price": price_id, "quantity": 1}],
            )

            return checkout_session["id"]
        except Exception as e:
            return str(e)

    def create_billing_session(self, customer_id: str, return_url: str) -> str:
        """
        Returns billing portal url.

        Args:
            customer_id (str): the stripe id of the user
            return_url (str): the url to redirect to upon leaving the billing portal

        Returns:
            json, int: Json containing billing url and status code
        """
        try:
            billing_session = stripe.billing_portal.Session.create(
                customer=customer_id, return_url=return_url
            )
            return billing_session.url
        except Exception as e:
            return str(e)


if __name__ == "__main__":
    cli = StripeClient(
        "sk_test_51IigLsL2k8k1yyGOHRL6meRZrA2S9SKc06p27x9ltjm1UQ09JSGGzjzu25vsIBiqLXbgOjhlRAlZ8gAG2iwaWGoI00xlRGhNWw"
    )
    # print(cli.time_left_in_paid_access("cus_J2NZ6LS5vuCqsh"))
