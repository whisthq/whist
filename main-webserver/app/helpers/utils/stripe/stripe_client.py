import logging
import traceback

from datetime import datetime, timedelta

from functools import reduce

import stripe
import uuid

from dateutil.relativedelta import relativedelta
from flask import jsonify
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

        self.user_schema = UserSchema
    
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

    def create_subscription(self, token, email, plan, code=None):
        """This will create a new subscription for a client with the given email
        and a code if it exists. This is similar to what was previously stripe_post's "chargeHelper."

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
        """

        # get the customer and whether they exist
        user = User.query.get(email)
        if not user:
            raise NonexistentUser
        
        try:
            token = stripe.Token.retrieve(token)
        except:
            raise InvalidStripeToken

        stripe_customer_id = customer.stripe_customer_id

        # get the zipcode/region 
        zipcode = token["card"]["address_zip"]

        if zipcode in self.zipcode_db:
            raise RegionNotSupported
        try:
            purchase_region_code = self.zipcode_db[zipcode].state
        except IndexError:
            raise RegionNotSupported

        purchase_region = self.regions[purchase_region_code].strip().lower()

        # apply tax rate for their region (find the id for their region)
        tax_rates = map(
            lambda tax: {
                "region" : tax["jurisdiction"].strip().lower(),
                "id": tax["id"]
            }, 
            stripe.TaxRate.list(limit=100, active=True)["data"]
        )
        tax_rate = reduce(
            lambda acc, tax: acc = acc if acc else tax["id"] if tax["region"] == purchase_region, 
            tax_rates, 
            None
        )

        # if they are a new customer initialize them
        # in either case find it whether we need to make a new subscription
        subscribed = True

        if not stripe_customer_id:
            customer = stripe.Customer.create(email=email, source=token)
            stripe_customer_id = new_customer["id"]
            referrer = User.query.filter_by(referral_code=code).first() if code else None

            
            trial_end = round((dt.now() + relativedelta(months=1)).timestamp()) if referrer else round((datetime.now() + timedelta(weeks=1)).timestamp())
            subscribed = False

            fractalLog(function="StripeClient.create_subscription", label=email, logs="Customer added successful")
        else:
            subscriptions = stripe.Subscription.list(customer=customer_id)["data"]
            if len(subscriptions) == 0:
                trial_end = round((dt.now() + relativedelta(weeks=1)).timestamp())
                subscribed = False
        
        # if they had a subscription modify it, don't make a new one
        # else make a new one with the corresponding params
        if subscriptions:
            stripe.SubscriptionItem.modify(
                    subscriptions[0]["items"]["data"][0]["id"], plan=PLAN_ID
                )
            fractalLog(function="StripeClient.create_subscription", label=email, logs="Customer updated successful")
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
                