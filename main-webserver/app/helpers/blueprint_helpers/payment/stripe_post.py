from flask import jsonify

from functools import reduce

from app.constants.http_codes import (
    BAD_REQUEST,
    CONFLICT,
    NOT_ACCEPTABLE,
    NOT_FOUND,
    PAYMENT_REQUIRED,
    SUCCESS,
    FORBIDDEN,
)
from app.helpers.utils.general.logs import fractalLog
from app.helpers.utils.payment.stripe_client import (
    StripeClient,
    NonexistentUser,
    RegionNotSupported,
    InvalidStripeToken,
    InvalidOperation,
)

# TODO (optional) make templates for boilerplate
def addSubscriptionHelper(token, email, plan, code):
    fractalLog(
        function="addSubscriptionHelper",
        label=email,
        logs="Signing {} up for plan {}, with code {}, token{}".format(email, plan, code, token),
    )
    client = StripeClient(STRIPE_SECRET)
    plans = client.get_prices()  # product Fractal by default

    plan = reduce(lambda acc, pl: acc if pl[0] != plan else pl[1], plans, None)
    if plan:
        try:
            client.create_subscription(token, email, plan, code=code)
            status = SUCCESS
        except NonexistentUser:
            status = FORBIDDEN
        except RegionNotSupported:
            status = BAD_REQUEST
        except InvalidStripeToken:
            status = BAD_REQUEST
        except Exception:
            status = INTERNAL_SERVER_ERROR
    else:
        status = BAD_REQUEST

    return jsonify({"status": status}), status


def deleteSubscriptionHelper(email):
    client = StripeClient(STRIPE_SECRET)
    try:
        client.cancel_subscription(email)
        return jsonify({"status": SUCCESS}), SUCCESS
    except NonexistentUser:
        return jsonify({"status": FORBIDDEN}), FORBIDDEN
    except InvalidOperation:
        return (
            jsonify({"status": NOT_ACCEPTABLE, "error": "Customer does not have a subscription!"}),
            NOT_ACCEPTABLE,
        )
    except Exception:
        fractalLog(
            function="deleteSubscriptionHelper", label=email, logs=str(e), level=logging.ERROR
        )
        return jsonify({"status": INTERNAL_SERVER_ERROR}), INTERNAL_SERVER_ERROR


def addCardHelper(email, token):
    client = StripeClient(STRIPE_SECRET)

    try:
        client.add_card(email, token)
        status = SUCCESS
    except NonexistentUser:
        status = FORBIDDEN
    except InvalidStripeToken:
        status = BAD_REQUEST
    except Exception:
        status = INTERNAL_SERVER_ERROR

    return jsonify({"status": status}), status


def deleteCardHelper(email):
    client = StripeClient(STRIPE_SECRET)

    try:
        client.delete_card(email)
        status = SUCCESS
    except NonexistentUser:
        status = FORBIDDEN
    except InvalidStripeToken:
        status = BAD_REQUEST
    except Exception:
        status = INTERNAL_SERVER_ERROR

    return jsonify({"status": status}), status


def retrieveHelper(email):
    client = StripeClient(STRIPE_SECRET)

    try:
        info = client.get_customer_info(email)
        info["status"] = SUCCESS
        return jsonify(info), SUCCESS
    except NonexistentUser:
        return jsonify({"status": FORBIDDEN}), FORBIDDEN
    # if info is None then we will raise a typerror
    except TypeError:
        return (
            jsonify(
                {
                    "status": PAYMENT_REQUIRED,
                    "subscription": {},
                    "cards": [],
                    "creditsOutstanding": credits,
                    "account_locked": False,
                    "customer": {},
                }
            ),
        )
        PAYMENT_REQUIRED,
    except Exception:
        return jsonify({"status": INTERNAL_SERVER_ERROR}), INTERNAL_SERVER_ERROR
