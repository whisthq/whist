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
    MONTHLY_PLAN_ID as monthly_plan,  # this is an old plan # TODO update this bad boy
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

# datetime evaluated once at the start so that we can check precisely the lengths
# these are used for referral codes
week = dateToUnix(datetime.now() + relativedelta(weeks=1))
month = dateToUnix(datetime.now() + relativedelta(months=1))


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


def _create_stripe_customer_and_subscribe(email, plan=monthly_plan, referrer=None):
    """Makes sure that there is a stripe customer with given email. If there is
    not one it will be created. Also adds the id to the database.

    Args:
        email (str): The email of the fractal (not stripe) user for whom we want
            for there to exist a stripe customer.
    """
    user = User.query.get(email)
    stripe_customer_id = user.stripe_customer_id

    if not stripe_customer_id:
        token = _generate_token()
        customer = stripe.Customer.create(email=email, source=token)
        stripe_customer_id = customer["id"]

        user.stripe_customer_id = stripe_customer_id
        db.session.commit()

        # we do it this way because referral_code for the same user may vary in production vs staging
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


@pytest.fixture
def client():
    """Makes a client with an api key (setting the global api key).

    Returns:
        (str): client with api key initialized.
    """
    return StripeClient(stripe_api_key)


@pytest.fixture()
def with_no_stripe_customer_id():
    """Makes sure that there is no stripe customer id before or after the function that uses this
    fixture.

    Yields:
        (bool): True
    """
    _remove_stripe_customer(dummy_email)
    yield True
    _remove_stripe_customer(dummy_email)


@pytest.fixture(params=[dummy_referrer, None])
def with_stripe_subscription(request):
    """Makes sure that there is a stripe customer id both before and after the the function
    that uses this fixture.

    Yields:
        (bool): True
    """
    _create_stripe_customer_and_subscribe(dummy_email, referrer=request.param)
    yield request.param
    _create_stripe_customer_and_subscribe(dummy_email, referrer=request.param)


"""Here we test the get_stripe_info for common cases."""


def test_get_stripe_info(client, with_stripe_subscription):
    referrer = with_stripe_subscription

    info = client.get_stripe_info(dummy_email)

    assert info["subscription"]["trial_end"] == (month if referrer else week)


def test_get_stripe_info_no_customer_id(client, with_no_stripe_customer_id):
    assert client.get_stripe_info(dummy_email) is None


"""Here we test creation of subscription for common cases."""

## TODO different combinations:
# 1. valid zip code vs invalid/international zip code
# 2. valid vs invalid token
# 3. existing user vs nonexistent user
# 4. existing stripe customer vs nonexistent stripe customer
# 5. with a valid code vs without a valid code vs no code
def test_create_subscription_without_customer(client, with_no_stripe_customer_id):
    dummy_token = _generate_token()

    assert client.create_subscription(dummy_token, dummy_email, monthly_plan)


# this will run twice, once with code, and once without referral code
def test_create_subscription_with_customer(client, with_stripe_subscription):
    dummy_token = _generate_token()
    referrer = with_stripe_subscription

    assert client.create_subscription(dummy_token, dummy_email, monthly_plan)

    user = User.query.get(dummy_email)
    stripe_customer_id = user.stripe_customer_id

    assert stripe.Subscription.list(customer=stripe_customer_id)["data"][0]["trial_end"] == (
        month if referrer else week
    )


"""Here we test subscruption cancellation for common cases."""


def test_cancel_subscription(client, with_stripe_subscription):
    assert client.cancel_subscription(dummy_email)


def test_cancel_no_subscription_throws(client, with_no_stripe_customer_id):
    with pytest.raises(InvalidOperation):
        client.cancel_subscription(dummy_email)


"""Here we test all functionality for invalid users."""


def test_invalid_user_throws(client):
    dummy_token = _generate_token()

    with pytest.raises(NonexistentUser):
        client.cancel_subscription(dummy_nonexistent_email)
    with pytest.raises(NonexistentUser):
        client.create_subscription(dummy_nonexistent_email, dummy_token, dummy_zip_us)
    with pytest.raises(NonexistentUser):
        client.get_stripe_info(dummy_nonexistent_email)
