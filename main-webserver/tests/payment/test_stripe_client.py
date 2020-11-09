import stripe

from tests.helpers.general.logs import fractalLog
from app.helpers.utils.payment.stripe_client import StripeClient
from app.constants.config import (
    MONTHLY_PLAN_ID as monthly_plan, # this is an old plan # TODO update this bad boy
    STRIPE_SECRET as stripe_api_key, # this is a test secret
)

stripe_no_auth_card = "4242424242424242"
stripe_auth_req_card = "4000002760003184"

email = "bob@tryfractal.com"

# def test_get_stripe_info():
#     client = StripeClient(stripe_api_key)

#     info = client.get_stripe_info(email)
#     fractalLog("", "", str(info))

#     assert True

## TODO set up fixtures or set up and tear down (xUnit?) style functions to make sure
# that the state is as desired for these tests
## TODO places to split at
# 1. valid zip code vs invalid/international zip code
# 2. valid vs invalid token
# 3. existing user vs nonexistent user
# 4. existing stripe customer vs nonexistent stripe customer
def test_create_subscription():
    client = StripeClient(stripe_api_key)

    token = stripe.Token.create(
        card={
            "address_zip": "08902", # a random zip code
            "number": stripe_no_auth_card,
            "exp_month": "12",
            "exp_year": "2021",
            "cvc": "123"
        },
    )

    # this is the expected format
    # otherwise you'll get a stripe.error.InvalidRequestError: Could not determine which URL to request
    # with "invalid ID"
    token = token.id 

    # TODO try with a code
    assert client.create_subscription(token, email, monthly_plan)

# def test_cancel_subscription():
#     client = StripeClient(stripe_api_key)

#     client.cancel_subscription(email)


if __name__ == "__main__":
    pass
