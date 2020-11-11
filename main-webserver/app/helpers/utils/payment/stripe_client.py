import logging

from datetime import datetime, timedelta

from functools import reduce

import stripe
import uuid

from flask import current_app

from dateutil.relativedelta import relativedelta
from pyzipcode import ZipCodeDatabase

# list of the 50 states
# { state_code : state_name } for all states
from app.constants.states import STATE_LIST

from app.helpers.utils.general.logs import fractalLog
from app.helpers.utils.general.time import dateToUnix, getToday

# TODO we may want to move this out of the client? not sure
from app.helpers.utils.mail.stripe_mail import creditAppliedMail

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


## TODO this class can be made clearer
## by seperating checks of existence of the client and subsequent actions
## by turning reused code into shared functions
## by removing methods that are not useful and creating helper methods for complex, useful methods
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

    def get_products(self, product_names=["Fractal"], limit=20):
        """Fetch the product ids of various products.

        Precondition:
            There are less than limit products to fetch. (i.e. you can fit on one page). This is
                realistic for Fractal.

        Args:
            product_names ([type], optional): [description]. Defaults to None.
            limit (int, optional): [description]. Defaults to 20.

        Returns:
            (generator[tuple[str, str]]): An iteratble of name_product_id for each of the products in
                product_names. For products that were not found returns None.
        """
        products = stripe.Product.list(limit=limit)["data"]
        product_names = set(product_names)

        for product in products:
            name, product_id = product["name"], product["id"]
            if name in product_names:
                product_names.remove(name)
                yield name, product_id

        for product_name in product_names:
            yield product_name, None

    def get_prices(self, products=["Fractal"]):
        """Fetch the prices of said products. This is a generator.

        Precondition:
            No product has more than 10 (default limit) prices in it. Each price has a metadata
                object with a key "name" mapping to a unique name.

        Precondition:
            Product names are unique.

        Args:
            products (list[str], optional): The iterable of products to get prices for.
                Defaults to ["Fractal"]. If the product is a product id it will simply fetch, otherwise
                it will get the product ids for the products and try again.

        Returns:
            (generator[tuple[str, str]]): An iterable of the name, price_id of each price (what used to be
                called "plans").
        """
        for product in products:
            try:
                for price in stripe.Price.list(product=product)[
                    "data"
                ]:  # TODO what if there are > 10
                    yield price["metadata"]["name"], price["id"]
            except:
                for _, product_id in self.get_products(product_names=[product]):
                    for price in stripe.Price.list(product=product)["data"]:
                        yield price["metadata"]["name"], price["id"]

    def get_customer_info(self, email):
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

            if subscription and subscription["data"] and len(subscription["data"]) > 0:
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

    # TODO make it so that if there is a source it will charge with their source
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
            subscriptions = None  # this is necessary since if <unbound> != if None (i.e. false)
            customer = stripe.Customer.create(email=email, source=token)
            stripe_customer_id = customer["id"]
            referrer = User.query.filter_by(referral_code=code).first() if code else None
            # they are rewarded by another request to discount by the client where they get credits

            if referrer:
                trial_end = dateToUnix(datetime.now() + relativedelta(months=1))
                self.discount(referrer)
            else:
                trial_end = dateToUnix(datetime.now() + timedelta(weeks=1))

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
        """Cancels the subscription for the user identified by the given
        email.

        Args:
            email (str): The email of the user whose subscription we are cancelling.

        Raises:
            NonexistentUser: If the user does not exist.
            InvalidOperation: If the user is not a stripe customer i.e. cannot have bought our product.
            InvalidOperation: If the user does not have any subscriptions.

        Returns:
            [type]: [description]
        """
        user = User.query.get(email)
        if not user:
            raise NonexistentUser

        if not user.stripe_customer_id:
            raise InvalidOperation

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

    def add_card(self, email, source):
        """Adds a card/source to the customer/user with the given email. If they do
            not have a source i.e. are not a customer (whenever we create a customer we always
            have a default source enter, so they should always have a source... unless they call
            delete_card enough, in which case I'm not sure) we create a new customer and a new source.
            We then store their customer id.

        Args:
            email (str): The email identifying the said customer.
            source (str): The source token/id (usually a card) for the payment method.

        Raises:
            NonexistentUser: If the user does not exist.
        """
        user = User.query.get(email)
        if not user:
            raise NonexistentUser

        if not user.stripe_customer_id:
            customer = stripe.Customer.create(email=email, source=token)

            user.stripe_customer_id = stripe_customer_id
            db.session.commit()
        else:
            stripe.Customer.create_source(user.stripe_customer_id, source=source)

    def delete_card(self, email, card):
        """Deletes a card (source) for a customer. This is not a recommended
        method to use. I'm not sure if it is necessary or what will happen if they try
        to delete their only source regarding the subscription.

        Args:
            email (str): Email for the user.
            card (str): Card token (or "id") for the card we want to remove.

        Raises:
            NonexistentUser: If the user doesn't exist.
            InvalidOperation: If the user does not have a stripe customer id i.e. has not signed up
                with a source yet.
        """
        user = User.query.get(email)
        if not user:
            raise NonexistentUser

        if not user.stripe_customer_id:
            raise InvalidOperation

        stripe.Customer.delete_source(user.stripe_customer_id, card)

    def discount(self, referrer):
        """Apply a discount to a referrer if they have referred. As you may notice,
        this isn't, strictly speaking, stripe code. The reason we keep it in the client
        is to abstract away this functionality from elsewhere, since it may at some point
        actually become stripe code.

        Args:
            referrer (str): The referrer who referred someone else such that we now want
                to discount them.

        Returns:
            (bool): False if there was an error discounting (i.e. there is no referrer) else True.
        """
        if not referrer:
            fractalLog(
                function="StripeClient.discount",
                label="None",
                logs="No user for code {}".format(code),
                level=logging.ERROR,
            )
            return False
        else:
            credits_outstanding = referrer.credits_outstanding
            if not credits_outstanding:
                credits_outstanding = 0
            email = referrer.user_id

            referrer.credits_outstanding = credits_outstanding + 1
            db.session.commit()

            fractalLog(
                function="StripeClient.discount",
                label=email,
                logs="Applied discount and updated credits outstanding",
            )

            # don't want to spam ming :)
            if not current_app.testing:
                creditAppliedMail(email)

            return True
