"""Simple testing framework for our stripe/payment endpoints. This mocks the stripe client, and thus
does not actually test it end to end. The unmocked testing of stripe functionality is in the
test_stripe_client tests. This way we can logically seperate tests of endpoints' adherence to the design
with tests of the actual stripe code."""

from app.constants.http_codes import (
    SUCCESS
)

from tests.conftest import (
    app,
    authorized,
    # TODO client?!
)

dummy_token = ""
dummy_email = ""
dummy_plan = ""
dummy_code = ""

# should be ok to reuse since it covers all things
# if you want to be more careful and see that each thing only relies
# on a few of the items then we can change it
dummy_body = dict(
    email=dummy_email,
    token=dummy_token,
    plan=dummy_plan,
    code=dummy_code,
)

def test_addSubscription_success(client, authorized):
    resp = client.post(
        "/stripe/addSubscription",
        json=dummy_body,
    )

    assert resp["status"] == SUCCESS

def test_addSubscription_failure(client, authorized):
    pass # TODO

def test_modifySubscription():
    pass # TODO

def test_deleteSubscription():
    pass # TODO

def test_addCard():
    pass # TODO

def test_modifyCard():
    pass # TODO

def test_deleteCard():
    pass # TODO

def test_retrieve():
    pass # TODO