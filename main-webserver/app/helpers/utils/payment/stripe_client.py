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
from app.helpers.utils.mail.stripe_mail import creditAppliedMail, planChangeMail

from app.models import db, User, LoginHistory
from app.serializers.public import UserSchema

""" Welcome to the stripe client. The stripe client is a one stop shop for all things stripe, similarly
to how the ECSClient covers much of our ECS core functionality, allowing us to use its various abstractions
for important tasks.

The Stripe client is designed to emulate previous stripe functionality, while cleaning up code and
provding users with easy to-use functions and descriptive errors defined below.

This client was written before any pricing meeting for the new container stack, so it may require some
updates in the near future. As of now it makes the same assumption as previous implementations, which
should be clear to avoid strange errors. If these assumptions are wrong in the future the code may need
to be changed.

Assumptions:
1. Every user is subscribed to at most one subscription.
2. Users have at least one source of payment.
3. There are <20 (though easy to change for <100) total prices available.
4. There are <10 (though easy to change for <100) total products availble. Best if only one.
5. Product names are unique.
6. Prices have, inside their metadata (look at the key-value metadata in the stripe dashboard) an item
    of the form "name" : "some stripe name"
7. The Stripe prices we use are: 
    7.1 "Fractal Hourly" (used to charge an additional subscription at a lower rate than the other ones)
    7.2 "Fractal Hourly Per Hour" (used to calculate how much to charge per hour of use)
    7.3 "Fractal Monthly" (used to charge a monthly subscription; all subscriptions are monthly)
    7.4 "Fractal Unlimited" (used to charge a subscription at a higher rate)
    Note that these are hardcoded into the code. If we grow to have more, it will help to have
    some set labels in a file to keep track (we'd map dynamically once to stripe objects).
8. The functions marked as "Deprecated" are not to be used without testing that they work and
    making edits at necessary. They are marked at the top of their doc string.

Future TODO/Tech Debt:
1. Make code generally more clear and less cluttered, removed unecessary functionality.
2. Batch checks of validity and then turn that into its own function to shorten code
3. Reuse boilerplate
4. Consolidate everything that is called "plan" and everything called "price" and just pick one (preferably
    price since plan might get deprecated by Stripe).
5. Create helper functions, and give more options to some of the fetch functionality so that you can
    get the entire objects or just the ids by choice, rather than being forced to only get the ids.
6. Make it so that if there is a source it will charge with their source and not need a token for
    create_subscription. In General, it may be good to do this and similar stuff for various functions,
    so having a decorator might be good.
"""


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

    def validate_customer_id(self, stripe_customer_id, user=None):
        """Confirms that a stripe customer id that we store is valid and is not
        deleted, for example. The use case is when we manually delete people from
        the stripe dashboard, we don't want to break the code (the db isn't updated immediately).

        Then it will delete a stripe_customer_id from the database if it does not exist (and user object
        is passed).

        Args:
            stripe_customer_id (str): The customer id string of the stripe customer, might be none.
            user (User, optional): The user for whom we might want to update the db.

        Returns:
            False if the stripe_customer_id was not valid even if we are able to update it (to be null).

        Raises:
            stripe.error.InvalidRequestError if customer is not retrievable (not a valid id) and we
            are not able to fix it.
        """
        if not stripe_customer_id:
            return True

        if stripe_customer_id:
            try:
                stripe.Customer.retrieve(stripe_customer_id)
            except stripe.error.InvalidRequestError as e:
                if user:
                    user.stripe_customer_id = None
                    db.session.commit()
                    return False
                else:
                    raise e
            return True

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
        products = stripe.Product.list(limit=limit, active=True)["data"]
        product_names = set(product_names) if product_names else None

        for product in products:
            name, product_id = product["name"], product["id"]

            if product_names is None:
                yield name_product_id
            elif name in product_names:
                product_names.remove(name)
                yield name, product_id

        for product_name in product_names:
            yield product_name, None

    def get_prices(self, products=["Fractal"]):
        """Fetch the prices of said products. This is a generator. Pass None as products for all.

        Precondition:
            No product has more than 10 (default limit) prices in it. Each price has a metadata
                object with a key "name" mapping to a unique name.

        Precondition:
            Product names are unique, less than limit (20) prices.

        Args:
            products (list[str], optional): The iterable of products to get prices for.
                Defaults to ["Fractal"]. If the product is a product id it will simply fetch, otherwise
                it will get the product ids for the products and try again.

        Returns:
            (generator[tuple[str, str]]): An iterable of the name, price_id of each price (what used to be
                called "plans").
        """
        if not products:
            # TODO key error use nickname?
            yield from map(
                lambda price: (price["metadata"]["name"], price["id"]),
                stripe.Price.list(limit=20, active=True)["data"],
            )
        else:
            for product in products:
                try:
                    for price in stripe.Price.list(product=product, active=True)[
                        "data"
                    ]:  # TODO what if there are > 10
                        yield price["metadata"]["name"], price["id"]
                except:
                    for _, product_id in self.get_products(product_names=[product]):
                        for price in stripe.Price.list(product=product_id, active=True)["data"]:
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

        stripe_customer_id = customer["stripe_customer_id"]

        # return a valid object if they exist else None
        if stripe_customer_id and self.validate_customer_id(stripe_customer_id, user):
            customer_info = stripe.Customer.retrieve(stripe_customer_id, expand=["sources"])
            cards = customer_info["sources"]["data"]

            subscription = stripe.Subscription.list(customer=stripe_customer_id)

            if subscription and subscription["data"] and len(subscription["data"]) > 0:
                subscription = subscription["data"][0]
                account_locked = subscription["trial_end"] < dateToUnix(getToday())
            else:
                subscription = None
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
            token (str): A stripe token that can be used to get credit card information etc by stripe.  It
                can be none to use the default source of an existing user.
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

        # if they pass an invalid string token then we raise an error otherwise we set it to
        # the token object (i.e. with the info etc)
        if not token is None:
            try:
                token = stripe.Token.retrieve(token)
            except:
                raise InvalidStripeToken

        stripe_customer_id = user.stripe_customer_id
        if not self.validate_customer_id(stripe_customer_id, user):
            stripe_customer_id = None

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
            # if they are not a customer they require a token
            if token is None:
                raise InvalidStripeToken

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
            # we have not yet allowed customers to delete cards, but
            # in the future when we do we should either not let them go below one, or
            # delete their stripe info if they do, or add something here to block you from
            # subscriping with a None token... I'm assuming they have a "default_source" in
            # their customer object
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

        stripe_customer_id = user.stripe_customer_id

        if not stripe_customer_id or not self.validate_customer_id(stripe_customer_id, user):
            raise InvalidOperation

        subscription = stripe.Subscription.list(customer=stripe_customer_id)["data"]

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

        stripe_customer_id = user.stripe_customer_id
        if not stripe_customer_id or not self.validate_customer_id(stripe_customer_id, user):
            customer = stripe.Customer.create(email=email, source=source)
            user.stripe_customer_id = customer["id"]
            db.session.commit()
        else:
            stripe.Customer.create_source(stripe_customer_id, source=source)

    def delete_card(self, email, source):
        """Deletes a card (source) for a customer. This is not a recommended
        method to use. I'm not sure if it is necessary or what will happen if they try
        to delete their only source regarding the subscription.

        In the future make it so you can pass in a card id instead as an alternative.

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

        stripe_customer_id = user.stripe_customer_id
        if not stripe_customer_id or not self.validate_customer_id(stripe_customer_id, user):
            raise InvalidOperation

        card = stripe.Token.retrieve(source)["card"]
        last4 = card["last4"]

        remove_ids = set()

        # if we make get customer info more flexible use it here
        customer_info = stripe.Customer.retrieve(stripe_customer_id, expand=["sources"])
        cards = customer_info["sources"]["data"]

        for card in cards:
            if card["last4"] == last4:
                remove_ids.add(card["id"])

        for card in remove_ids:
            stripe.Customer.delete_source(user.stripe_customer_id, card)

    def discount(self, referrer):
        """Apply a discount to a referrer if they have referred. As you may notice,
        this isn't, strictly speaking, stripe code. The reason we keep it in the client
        is to abstract away this functionality from elsewhere, since it may at some point
        actually become stripe code.

        Args:
            referrer (app.models.User): The referrer who referred someone else such that we now want
                to discount them.

        Returns:
            (bool): False if there was an error discounting (i.e. there is no referrer) else True.
        """
        if not referrer:
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

    def charge_hourly(self, email):
        """Deprecated.

        A function that we should be calling just after a user logs off to charge them
        for the mount of hours they were logged on to Fractal.

        Currently, if they use Fractal for >0 minutes, we round their usage time to a minimum of one unit,
        since Stripe does not seem to allow fractional units. In the future, we may prefer to simply
        change the per-hour monthly plan and then add a table in the db for these users and how many
        minutes they've used Fractal for. (this is a TODO and kind of a big one at that).

        Moreover, this uses the charges api which is not compliant with certain EU regulations. Effectively,
        you'd need to switch over the the Payment Intent API to be able to go beyond NA. (Keep in mind
        that this also does not actually use the Fractal Hourly Per Hour to avoid clutter.)

        Args:
            email (str): The email of the user that just logged off.

        Raises:
            NonexistentUser: The user does not exist.
            RuntimeError: Somehow the hourly fractal plan/price does not exist, we may have
                moved on in our pricing strategy and this function is deprecated. Similarly, if
                the Fractal Hourly per-hour does not exist we will raise a similar error.
            InvalidOperation: The user is not an hourly price user. They should not be charged
                per the hour. Also raised if the user is not a stripe user. Also raised if they
                do not have a source.
        """
        # Check to see if the user is a current customer
        user = User.query.get(email)

        if not customer:
            raise NonexistentUser

        # Check to see if user is an hourly plan subscriber

        hourly_price = None
        for price_name, price in self.get_prices():
            if price_name == "Fractal Hourly":
                hourly_price = price
                break

        if not hourly_price:
            raise RuntimeError("Hourly price option not found at all on Fractal Stripe.")

        stripe_customer_id = user.stripe_customer_id

        if not stripe_customer_id:
            raise InvalidOperation

        subscriptions = stripe.Subscription.list(customer=customer["stripe_customer_id"])["data"]
        subscription = subscriptions[0]

        up = lambda p: subscription["items"]["data"][0][p]["id"]
        try:
            user_price = up("price")
        except:
            user_price = up("plan")

        if user_price != hourly_price:
            fractalLog(
                function="StripeClient.charge_hourly",
                label=email,
                logs="{username} is not an hourly price subscriber. Why are we charging them hourly?".format(
                    username=email
                ),
                level=logging.ERROR,
            )
            return

        latest_user_activity = (
            LoginHistory.query.filter_by(user_id=email)
            .order_by(LoginHistory.timestamp.desc())
            .first()
        )

        if latest_user_activity.action != "logon":
            fractalLog(
                function="StripeClient.charge_hourly",
                label=email,
                logs="{username} logged off and is an hourly subscriber, but no logon was found".format(
                    username=email
                ),
                level=logging.ERROR,
            )
            return

        now = datetime.now()
        logon = datetime.strptime(user_activity["timestamp"], "%m-%d-%Y, %H:%M:%S")

        hours_used = now - logon

        if hours_used > timedelta(minutes=0):
            prices = self.get_prices()
            # unlike above, this is for the actual rate we'll be charging per hour
            # we also charge them $5.00 per month which is why the per-hour pricing strategy
            # has two prices
            hourly_price_per_hour = None
            for price_name, price in prices:
                if price_name == "Fractal Hourly Per Hour":
                    hourly_price_per_hour = price
                    break

            if not hourly_price_per_hour:
                raise RuntimeError("Hourly price per-hour price does not exist on Fractal Stripe.")

            # TODO self.get_prices should give you the option to fetch this
            price_amount_cents = stripe.Price.retrieve(hourly_price_per_hour)["unit_amount"]
            # 60 seconds per minute * 60 minutes per hour = per hour
            total_hours = hours_used.total_seconds() / 3600
            total_price = round(total_hours * price_amount_cents)

            # TODO this can be simplified
            source = stripe.Customer.retrieve(stripe_customer_id)["default_source"]
            if not source:
                source = self.get_customer_info(email)["cards"][0]["data"]["id"]

            stripe.Charge.create(
                amount=total_price,
                currency="usd",
                source=source,
                description="Charging for hourly use. User {username} used for {hours_used} hours. Charging {rate} per hour.".format(
                    username=email, hours_used=str(total_hours), rate=price_amount_cents
                ),
            )

            fractalLog(
                function="StripeClient.charge_hourly",
                label=email,
                logs="{username} used Fractal for {hours_used} hours and is an hourly subscriber. Charged {amount} cents".format(
                    username=email, hours_used=str(total_hours), amount=total_price
                ),
            )

        def add_product(self, email, product_name, price_name):
            """Deprecated.

            The will add a product to a user if the price with price_name as its name is
            inside the product.

            Args:
                email (str): Email of user we are adding product to.
                product_name (str): Name of the product that you can see in the stripe dashboard.
                price_name (str): Name of the price we want to identify by.

            Raises:
                NonexistentUser: The user does not exist.
                InvalidOperation: The user is not a stripe customer.
            """
            user = User.query.get(email)
            if not user:
                raise NonexistentUser

            stripe_customer_id = user.stripe_customer_id

            if not stripe_customer_id:
                raise InvalidOperation

            price = None
            for _price_name, _price in self.get_prices(products=[product_name]):
                if price_name == _price_name:
                    price = _price
                    break

            if not price:
                raise RuntimeError(
                    "Tried to add product with a price/plan, but price didn't exist."
                )

            subscription = stripe.Subscription.list(customer=stripe_customer_id)["data"][0]
            subscription_item = None
            for item in subscription["items"]["data"]:
                if item["price"]["id"] == price:  # plan or price?
                    subscription_item = stripe.SubscriptionItem.retrieve(item["id"])
                    break

            if not subscription_item:
                stripe.SubscriptionItem.create(subscription=subscription["id"], price=price)
            else:
                stripe.SubscriptionItem.modify(
                    subscription_item["id"], quantity=subscription_item["quantity"] + 1
                )

        def remove_product(self, email, product_name, price_name):
            """Deprecated.

            Almost the same as add_product, except will remove one.

            Args:
                email (str): The email of the user for which we want to remove product
                    named by product_name.
                product_name (str): Name of the product we want to remove.
                price_name (str): Name of the price we want to identify by.

            Raises:
                NonexistentUser: User does not exist.
                InvalidOperation: User is not a stripe customer.
                RuntimeError: Price that
            """
            user = User.query.get(email)

            if not user:
                raise NonexistentUser

            stripe_customer_id = customer.stripe_customer_id
            if not stripe_customer_id:
                raise InvalidOperation

            price = None
            for _price_name, _price in self.get_prices(products=[product_name]):
                if price_name == _price_name:
                    price = _price
                    break

            if not price:
                raise RuntimeError(
                    "Tried to remove product with a price/plan, but price didn't exist."
                )

            subscription = stripe.Subscription.list(customer=customer_id)["data"][0]
            subscription_item = None
            for item in subscription["items"]["data"]:
                if item["price"]["id"] == price:  # price or plan?
                    subscription_item = stripe.SubscriptionItem.retrieve(item["id"])
                    break

            if subscription_item:
                if subscription_item["quantity"] == 1:
                    stripe.SubscriptionItem.delete(subscription_item["id"])
                else:
                    stripe.SubscriptionItem.modify(
                        subscription_item["id"], quantity=subscription_item["quantity"] - 1
                    )

        def updateHelper(self, email, new_price_name):
            """Deprecated.

            Update the plan/price for a certain user to by that with the new_price_name name.

            Args:
                email (str): Email of the user.
                new_price_name (str): Name of the new price/plan they want.

            Raises:
                RuntimeError: If the new price does not exist.
                NonexistentUser: If the user does not exist.
                InvalidOperation: If the user does not have a stripe customer id.
                InvalidOperation: If the user is not subscribed to anything.
            """
            new_price = None
            for price_name, price in self.get_prices():
                if price_name == new_price_name:
                    new_price = price
                    break

            if not new_price:
                raise RuntimeError("New price does not exist with given name.")

            user = User.query.get(email)
            if not user:
                raise NonexistentUser

            stripe_customer_id = user.stripe_customer_id
            if not stripe_customer_id:
                raise InvalidOperation

            subscriptions = stripe.Subscription.list(customer=stripe_customer_id)["data"]
            if not subscriptions or len(subscriptions) == 0:
                raise InvalidOperation

            stripe.SubscriptionItem.modify(
                subscriptions[0]["items"]["data"][0]["id"], price=new_price
            )

            if not current_app.testing:
                planChangeMail(username, new_plan_type)
