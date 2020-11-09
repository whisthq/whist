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

# TODO remove fractalLog after we are done making this work
#from tests.helpers.general.logs import fractalLog

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
    MONTHLY_PLAN_ID as monthly_plan, # this is an old plan # TODO update this bad boy
    UNLIMITED_PLAN_ID as unlimited_plan,
    STRIPE_SECRET as stripe_api_key, # this is a test secret
)

# the two test cards stripe gives us
stripe_no_auth_card = "4242424242424242"
stripe_auth_req_card = "4000002760003184"

# arbitrary zip codes in the uk and us respectivelly
dummy_zip_uk = "CR09XY"
dummy_zip_us = "08902"

# arbitrary 
dummy_email = "bob@tryfractal.com"
dummy_nonexistent_email = "sdfsd23498io2u3ihwe232342njkosjdfldsfsdfs12312kh"

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
        [type]: [description]
    """
    return stripe.Token.create(
        card={
            "address_zip": zipcode,
            "number": number,
            "exp_month": "12",
            "exp_year": "2021",
            "cvc": "123"
        },
    ).id if not malformed else "3489weji456rdfdfdbhjiosdn" # an arbitrary not a token string

## TODO set up fixtures or set up and tear down (xUnit?) style functions to make sure
# that the state is as desired for these tests

## TODO different combinations:
# 1. existing user vs nonexistent user
# 2. is a customer or is not
def test_get_stripe_info():
    client = StripeClient(stripe_api_key)

    info = client.get_stripe_info(dummy_email)

## TODO different combinations:
# 1. valid zip code vs invalid/international zip code
# 2. valid vs invalid token
# 3. existing user vs nonexistent user
# 4. existing stripe customer vs nonexistent stripe customer
# 5. with a valid code vs without a valid code vs no code
def test_create_subscription():
    client = StripeClient(stripe_api_key)

    # this is the expected format (i.e. a string: what you get from <token obj>.id)
    # otherwise you'll get a stripe.error.InvalidRequestError: 
    # Could not determine which URL to request & something about an "invalid ID"
    dummy_token = _generate_token()

    assert client.create_subscription(dummy_token, dummy_email, monthly_plan)

## TODO different combinations: 
# 1. existing user vs nonexistent user
# 2. user has stripe customer id vs doesn't
# 3. user has a subscription vs has no subscription
def test_cancel_subscription():
    client = StripeClient(stripe_api_key)

    assert client.cancel_subscription(dummy_email)


if __name__ == "__main__":
    pass
