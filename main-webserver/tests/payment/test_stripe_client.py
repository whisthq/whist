"""The stripe client is our one stop shop for all things stripe. It encapsulates various different
flows of stripe functionality that we commonly use in creating and cancelling subscriptions, fetching
user information, rewarding users, etcetera. The goal is to abstract away raw Stripe API code (which,
despite being simple, would require future developers to go read docs, make readability worse, etc).

Here we test that its functions work as intended. This is not the same as testing end to end payments
(or endpoints themselves at all). The endpoints use the stripe client to do their thing, but may have
additional functionality. They are tested elsewhere.

To test stripe we use the test stripe secret which we store on the config database. We may be storing
various restricted keys in the future as well for different endpoints or whatever.

Since we want to make sure the client is talking properly to the server we do not mock requests. It is
easy to simply use the test key given to us by stripe.
"""

import stripe
import pytest

from tests.helpers.general.logs import fractalLog

from datetime import datetime
from dateutil.relativedelta import relativedelta
from app.helpers.utils.general.time import dateToUnix

from app.helpers.utils.payment.stripe_client import (
    StripeClient,
    # for when users are not in the db
    NonexistentUser,
    # for when the token is malformed
    InvalidStripeToken,
    # for when users are already subscribed and we try to do so again or are not and we try to cancel
    InvalidOperation,
    # for when the region is not supported
    RegionNotSupported,
)

from app.constants.config import (
    STRIPE_SECRET as stripe_api_key,  # this is a test secret
)

from app.models import db, User

# the two test cards stripe gives us
stripe_no_auth_card = "4242424242424242"
stripe_auth_req_card = "4000002760003184"

# arbitrary zip codes in the uk and us respectivelly
dummy_zip_uk = "CR09XY"
dummy_zip_us = "08902"

# arbitrary
dummy_email = "bob@tryfractal.com"
dummy_nonexistent_email = "sdfsd23498io2u3ihwe232342njkosjdfldsfsdfs12312kh"
dummy_referrer = "ming@tryfractal.com"  # make sure we don't remove ming

dummy_invalid_referral_code = "asdofhasiuldofjlasnklfd23980q9wsiojdnh"

# datetime evaluated once at the start so that we can check precisely the lengths
# these are used for referral codes
week = dateToUnix(datetime.now() + relativedelta(weeks=1))
month = dateToUnix(datetime.now() + relativedelta(months=1))


def _get_monthly_plan():
    stripe.api_key = stripe_api_key
    for price in stripe.Price.list(limit=20):
        metadata = price["metadata"]
        if "name" in metadata and metadata["name"] == "Fractal Monthly":
            return price["id"]
    return None  # should not be reached


monthly_plan = _get_monthly_plan()


def _generate_token(number=stripe_no_auth_card, zipcode=dummy_zip_us, malformed=False):
    """Helper function to generate a token string that we can use to test subscription
    creation and other whatnot.

    Precondition:
        You MUST have already called stripe.api_key = <my api key> for this to work. To this end
            either initialize a StripeClient with the stripe_api_key we imported or manually set
            the key to be the stripe secret imported above.

    Args:
        number (str, optional): The card number. Defaults to stripe_no_auth_card.
        zipcode (str, optional): The zip code of the card; necessary to test for
            whether users are in the us or not. Defaults to dummy_zip_us.

    Returns:
        (str): The token. It can be malformed it not malformed.
    """

    # this is the expected format (i.e. a string: what you get from <token obj>.id)
    # otherwise you'll get a stripe.error.InvalidRequestError:
    # Could not determine which URL to request & something about an "invalid ID"
    return (
        stripe.Token.create(
            card={
                "address_zip": zipcode,
                "number": number,
                "exp_month": "12",
                "exp_year": "2021",
                "cvc": "123",
            },
        ).id
        if not malformed
        else "3489weji456rdfdfdbhjiosdn"
    )  # an arbitrary not a token string


# for i in range(100):
#     fractalLog("Generated Token:", f"{i}", _generate_token())


def _remove_stripe_customer(email):
    """Makes sure there is no stripe customer with given email. If there is
    the user is deleted. Also removes user id from database.

    Args:
        email (str): the email of the fractal (not stripe) user for whom we want to remove
            their stripe customer id.
    """
    user = User.query.get(email)
    stripe_customer_id = user.stripe_customer_id

    if stripe_customer_id:
        # their subscriptions should go away with this too
        stripe.Customer.delete(stripe_customer_id)

        user.stripe_customer_id = None
        db.session.commit()


def _new_customer(email, plan=monthly_plan, referrer=None, subscribe=True):
    """Makes sure that there is a stripe customer with given email. If there is
    not one it will be created. Also adds the id to the database.

    Args:
        email (str): The email of the fractal (not stripe) user for whom we want
            for there to exist a stripe customer.
    """
    user = User.query.get(email)
    stripe_customer_id = user.stripe_customer_id

    if stripe_customer_id:
        _remove_stripe_customer(email)

    token = _generate_token()
    customer = stripe.Customer.create(email=email, source=token)
    stripe_customer_id = customer["id"]

    user.stripe_customer_id = stripe_customer_id
    db.session.commit()

    if subscribe:
        # we do it this way because referral_code for the same user may vary in production vs staging
        if referrer:
            referrer = User.query.get(referrer)
            if not referrer.referral_code:
                referrer = None
        trial_end = month if referrer else week

        # keep trial_end so that we can test that it was correctly evaluated based on referrer
        stripe.Subscription.create(
            customer=stripe_customer_id,
            items=[{"plan": plan}],
            trial_end=trial_end,
        )


def _assert_create_get_trial_end(referrer, client):
    """This helper function is commonly used in create to check that it runs through, returning the
    trial end to be checked for validity.

    Args:
        referrer (str): Email of the user who referred the dummy user to fractal.

    Returns:
        (unix time): The time of the end of the trial.
    """
    dummy_token = _generate_token()
    referral_code = _get_code(referrer)

    assert client.create_subscription(dummy_token, dummy_email, monthly_plan, code=referral_code)

    stripe_customer_id = _get_stripe_customer_id()
    return stripe.Subscription.list(customer=stripe_customer_id)["data"][0]["trial_end"]


def _get_code(referrer):
    if not referrer:
        return None

    return User.query.get(referrer).referral_code


def _get_stripe_customer_id(email=dummy_email):
    user = User.query.get(dummy_email)
    return user.stripe_customer_id


def _closest(val, options):
    return min(options, key=lambda option: abs(val - option))


@pytest.fixture
def client():
    """Makes a client with an api key (setting the global api key).

    Returns:
        (str): client with api key initialized.
    """
    return StripeClient(stripe_api_key)


@pytest.fixture()
def not_customer():
    """Makes sure that there is no stripe customer id before or after the function that uses this
    fixture.

    Yields:
        (bool): True
    """
    _remove_stripe_customer(dummy_email)
    yield True
    _remove_stripe_customer(dummy_email)


"""Here we test the get_customer_info for common cases. These include whether you've subscribed or not."""


def test_get_stripe_info(client, not_customer):

    for subscribe in [True, False]:
        for referrer in [dummy_referrer, None]:

            _new_customer(dummy_email, referrer=referrer, subscribe=subscribe)

            info = client.get_customer_info(dummy_email)

            if subscribe:
                trial_end = info["subscription"]["trial_end"]
                closest = _closest(trial_end, [month, week])
                assert closest == month if referrer else week
            else:
                assert info["subscription"] is None

            _remove_stripe_customer(dummy_email)


def test_get_stripe_info_no_customer_id(client, not_customer):
    assert client.get_customer_info(dummy_email) is None


"""Here we test creation of subscription for common cases."""


def test_create_subscription_with_not_customer(client, not_customer):
    for referrer in [dummy_referrer, None]:
        trial_end = _assert_create_get_trial_end(referrer, client)

        assert _closest(trial_end, [month, week]) == month if referrer else week

        _remove_stripe_customer(dummy_email)


def test_create_subscription_with_customer(client, not_customer):

    for original_referrer in [dummy_referrer, None]:
        for subscribed in [True, False]:
            _new_customer(dummy_email, referrer=original_referrer, subscribe=subscribed)

            for referrer in [dummy_referrer, None]:
                trial_end = _assert_create_get_trial_end(referrer, client)

                assert (
                    _closest(trial_end, [month, week]) == month
                    if original_referrer and subscribed
                    else week
                )


def test_create_subscription_invalid_code(client, not_customer):
    dummy_token = _generate_token()

    assert client.create_subscription(
        dummy_token, dummy_email, monthly_plan, dummy_invalid_referral_code
    )

    stripe_customer_id = _get_stripe_customer_id()
    trial_end = stripe.Subscription.list(customer=stripe_customer_id)["data"][0]["trial_end"]

    assert _closest(trial_end, [month, week]) == week


def test_invalid_zip_code(client, not_customer):
    dummy_token = _generate_token(zipcode=dummy_zip_uk)

    with pytest.raises(RegionNotSupported):
        client.create_subscription(dummy_token, dummy_email, monthly_plan)


def test_invalid_token(client, not_customer):
    dummy_token = _generate_token(malformed=True)

    with pytest.raises(InvalidStripeToken):
        client.create_subscription(dummy_token, dummy_email, monthly_plan)


"""Here we test subscruption cancellation for common cases."""


def test_cancel_subscription(client, not_customer):
    for subscribe, referrer in [(True, dummy_referrer), (True, None), (False, None)]:
        _new_customer(dummy_email, referrer=referrer, subscribe=subscribe)

        if subscribe:
            assert client.cancel_subscription(dummy_email)
        else:
            with pytest.raises(InvalidOperation):
                client.cancel_subscription(dummy_email)

        _remove_stripe_customer(dummy_email)


# same problem as above
def test_cancel_no_stripe_throws(client, not_customer):
    with pytest.raises(InvalidOperation):
        client.cancel_subscription(dummy_email)


"""Here we test application of the discount in a valid case and an invalid case."""


def test_discount(client):
    # ming is going to have a hell of a lot of credits
    dummy = User.query.get(dummy_referrer)
    original_credits = dummy.credits_outstanding

    client.discount(dummy)
    dummy = User.query.get(dummy_referrer)
    new_credits = dummy.credits_outstanding

    assert new_credits == 1 or not original_credits is None and new_credits == original_credits + 1

    client.discount(None)
    dummy = User.query.get(dummy_referrer)
    newest_credits = dummy.credits_outstanding

    assert newest_credits == new_credits


"""Here we test functionality of adding a card and deleting a card for valid users and users without
stripe customer ids.
"""


def test_add_card(client, not_customer):
    dummy_token = _generate_token()
    get = lambda: User.query.get(dummy_email)
    info = lambda: stripe.Customer.retrieve(user.stripe_customer_id, expand=["sources"])

    # test for a user that does not have a customer id
    client.add_card(dummy_email, dummy_token)
    user = get()

    assert user.stripe_customer_id
    customer_info = info()
    cards = customer_info["sources"]["data"]  # might also have a default source but we don't care

    assert len(cards) == 1

    # this is allowed through somehow without requiring auth for this
    # probably because it's not being charged
    # anyways this card supposedly requires auth, but I needed another test card to see if this
    # would work :P
    dummy_token = _generate_token(number=stripe_auth_req_card)
    client.add_card(dummy_email, dummy_token)

    customer_info = info()
    cards = customer_info["sources"]["data"]
    assert len(cards) == 2


def test_delete_card_no_stripe_customer_id(client, not_customer):
    with pytest.raises(InvalidOperation):
        client.delete_card(dummy_email, "dummy")


def test_delete_card(client, not_customer):
    dummy_token = _generate_token()
    get = lambda: User.query.get(dummy_email)

    _new_customer(dummy_email, subscribe=False)

    user = get()
    info = lambda: stripe.Customer.retrieve(user.stripe_customer_id, expand=["sources"])

    customer_info = info()
    cards = customer_info["sources"]["data"]
    card = cards[0]["id"]

    client.delete_card(dummy_email, card)

    customer_info = info()
    cards = customer_info["sources"]["data"]
    assert len(cards) == 0


"""Here we will test fetch functions like get_prices and get_products. We'll test once with names and once
with ids for get_prices."""


def test_get_prices_with_ids(client):
    # for now Fractal is the only product, but whatever
    all_prices = set()
    all_prices_again = set()

    products = map(
        lambda p: p["id"],
        (p for p in stripe.Product.list(limit=10)["data"] if p["name"] == "Fractal"),
    )

    # fetch all prices by their product ids
    for price_name, price in client.get_prices(products=products):
        retrieved = stripe.Price.retrieve(price)
        assert not retrieved is None and len(retrieved) > 0

        all_prices.add(price)

    # fetch all prices
    for price_name, price in client.get_prices(products=None):
        assert price in all_prices

        all_prices.remove(price)
        all_prices_again.add(price)

    products = map(
        lambda p: p["name"],
        (p for p in stripe.Product.list(limit=10)["data"] if p["name"] == "Fractal"),
    )

    # fetch all prices by product name and not id
    for price_name, price in client.get_prices(products=products):
        assert price in all_prices_again

        all_prices_again.remove(price)


def test_get_products(client):
    pass  # TODO


"""Here we test all functionality for invalid users."""

# it's ok to ignore whether the state exists or not since this will exit
# before any modifications are made
def test_invalid_user_throws(client):
    dummy_token = _generate_token()

    with pytest.raises(NonexistentUser):
        client.cancel_subscription(dummy_nonexistent_email)
    with pytest.raises(NonexistentUser):
        client.create_subscription(dummy_nonexistent_email, dummy_token, dummy_zip_us)
    with pytest.raises(NonexistentUser):
        client.get_customer_info(dummy_nonexistent_email)
    with pytest.raises(NonexistentUser):
        client.add_card(dummy_nonexistent_email, "dummy")
    with pytest.raises(NonexistentUser):
        client.delete_card(dummy_nonexistent_email, "dummy")
