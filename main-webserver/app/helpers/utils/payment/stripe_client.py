import logging

from datetime import datetime, timedelta

from functools import reduce

import stripe
import uuid

from dateutil.relativedelta import relativedelta
from pyzipcode import ZipCodeDatabase

# list of the 50 states
# { state_code : state_name } for all states
from app.constants.states import STATE_LIST

from app.helpers.utils.general.logs import fractalLog
from app.helpers.utils.general.time import dateToUnix, getToday

from app.models import db, User
from app.serializers.public import UserSchema


class NonexistentUser(Exception):
    """This exception is raised when the user (customer) does not exist on a charge or
    simiar transaction."""

    pass


class RegionNotSupported(Exception):
    """We only suppor the NA (North America) region currently. In the past we only
    supported the US, though in the future we'd like to go global. This exception is
    raised when transactions are made for customers outside supported regions."""

    pass


class InvalidStripeToken(Exception):
    """This exception is raised when a customer passes an invalid token to an endpoint.
    For example, if they are trying to charge and pass in a token that stripe cannot parse
    we will raise this issue."""

    pass


class InvalidOperation(Exception):
    """This is raised when you try to delete your subscription if you are not subscribed
    or you try to create a subscription already being subscribed without modifying."""

    pass


class StripeClient:
    # api key must be the stripe secret key or
    # a restrited "secret" key as in the stripe dashboard
    # go to the keys section on the bottom left and search for "limitted access"
    def __init__(self, api_key):
        """Initialize a reusable StripeClient object that can be used to execute various
        actions for a client by our stripe endpoints.

        Args:
            api_key (str): A stripe private key, or a restricted access key (i.e. for micro-services,
                which may actually end up being what we use) to allow for certain types of actions to
                be taken.
        """
        # this is not saved in our class for security reasons
        # also we do not need it
        stripe.api_key = api_key

        self.regions = STATE_LIST
        self.zipcode_db = ZipCodeDatabase()

        self.user_schema = UserSchema()

    def refresh_key(self, api_key):
        """Reset the stripe api key.

        Args:
            api_key (str): A stripe api key that we want to set as
                the new value of the api key. We may want to do this
                if we are executing various commands or reusing an object
                in different endpoints, and don't want someone to be able
                to use one key to do various things (minimizing damage).
        """
        stripe.api_key = api_key

    def get_stripe_info(self, email):
        """Retrieves the information for a user regarding stripe. Returns in the following format:
        {
            "subscription": a subscription object per the stripe api (json format),
            "cards": cards object per stripe api (also json format),
            "creditsOutstanding": what it says,
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
        credits_outstanding = customer["credits_outstanding"]

        # return a valid object if they exist else None
        if customer["stripe_customer_id"]:
            customer_info = stripe.Customer.retrieve(customer["stripe_customer_id"])
            subscription = stripe.Subscription.list(customer=customer["stripe_customer_id"])

            if subscription and subscription["data"]:
                subscription = subscription["data"][0]
                cards = customer_info["sources"]["data"]
                account_locked = subscription["trial_end"] < dateToUnix(getToday())
            else:
                subscription = None
                cards = []
                account_locked = False

            return {
                "subscription": subscription,
                "cards": cards,
                "creditsOutstanding": credits_outstanding,
                "account_locked": account_locked,
                "customer": customer,
            }
        else:
            return None

    def create_subscription(self, token, email, plan, code=None):
        """This will create a new subscription for a client with the given email
        and a code if it exists. This is similar to what was previously stripe_post's "chargeHelper."
        It will return true on a succes run.

        Args:
            token (str): A stripe token that can be used to get credit card information etc by stripe.
            email (str): The user's email.
            plan (str): The plan they have purchased (can by our standard, or unlimited etc).
            code (str, optional): A promo/referall code if they were referred. None by default indicates
                not referrer and will not give them any referall advantages. A referred individual gets
                their first month free, while a regular user only gets one week (due to the stripe
                mechanisms at play which start charging after a week and then execute daily).

        Raises:
            NonexistentUser: If this is not a user i.e. does not have an account. Users who don't have
                a subscription already ARE users. This is for people who've never logged in to our site.
            InvalidStripeToken: The token is invalid or malformatted.
            RegionNotSupported: The region is not supported. We only support NA (really US) right now.

        Other exceptions may be raised in malformed data is passed in. This is meant to be caught outside.
        """

        # get the customer and whether they exist
        user = User.query.get(email)
        if not user:
            raise NonexistentUser

        try:
            token = stripe.Token.retrieve(token)
        except:
            raise InvalidStripeToken

        stripe_customer_id = user.stripe_customer_id

        # get the zipcode/region
        zipcode = token["card"]["address_zip"]

        if not self.zipcode_db[zipcode]:
            raise RegionNotSupported
        try:
            purchase_region_code = self.zipcode_db[zipcode].state
        except IndexError:
            raise RegionNotSupported

        purchase_region = self.regions[purchase_region_code].strip().lower()

        # apply tax rate for their region (find the id for their region)
        tax_rates = map(
            lambda tax: {"region": tax["jurisdiction"].strip().lower(), "id": tax["id"]},
            stripe.TaxRate.list(limit=100, active=True)["data"],
        )
        tax_rate = reduce(
            lambda acc, tax: tax["id"] if tax["region"] == purchase_region else acc, tax_rates, None
        )

        # if they are a new customer initialize them
        # in either case find it whether we need to make a new subscription
        subscribed = True

        if not stripe_customer_id:
            subscriptions = None # this is necessary since if <unbound> != if None (i.e. false)
            customer = stripe.Customer.create(email=email, source=token)
            stripe_customer_id = customer["id"]
            referrer = User.query.filter_by(referral_code=code).first() if code else None
            # they are rewarded by another request to discount by the client where they get credits

            trial_end = (
                dateToUnix(datetime.now() + relativedelta(months=1))
                if referrer
                else dateToUnix(datetime.now() + timedelta(weeks=1))
            )
            subscribed = False

            user.stripe_customer_id = stripe_customer_id
            user.credits_outstanding = 0
            db.session.commit()

            fractalLog(
                function="StripeClient.create_subscription",
                label=email,
                logs="Customer added successful",
            )
        else:
            subscriptions = stripe.Subscription.list(customer=stripe_customer_id)["data"]
            if len(subscriptions) == 0:
                trial_end = dateToUnix(datetime.now() + relativedelta(weeks=1))
                subscribed = False

        # if they had a subscription modify it, don't make a new one
        # else make a new one with the corresponding params
        if subscriptions:
            stripe.SubscriptionItem.modify(subscriptions[0]["items"]["data"][0]["id"], plan=plan)
            fractalLog(
                function="StripeClient.create_subscription",
                label=email,
                logs="Customer updated successful",
            )
        else:
            stripe.Subscription.create(
                customer=stripe_customer_id,
                items=[{"plan": plan}],
                trial_end=trial_end,
                trial_from_plan=False,
                default_tax_rates=[tax_rate],
            )
            fractalLog(
                function="StripeClient.create_subscription",
                label=email,
                logs="Customer subscription created successful",
            )

        return True

    def cancel_subscription(self, email):
        user = User.query.get(email)
        if not user:
            raise NonexistentUser

        subscription = stripe.Subscription.list(customer=user.stripe_customer_id)["data"]

        if len(subscription) == 0:
            raise InvalidOperation
        else:
            stripe.Subscription.delete(subscription[0]["id"])
            fractalLog(
                function="StripeClient.cancel_subscription",
                label=email,
                logs="Cancelled stripe subscription for {}".format(email),
            )
            return True
