import logging

from flask import current_app, jsonify

from functools import reduce

from app.constants.http_codes import (
    BAD_REQUEST,
    NOT_ACCEPTABLE,
    PAYMENT_REQUIRED,
    SUCCESS,
    FORBIDDEN,
    INTERNAL_SERVER_ERROR,
)
from app.helpers.utils.general.logs import fractal_log
from app.helpers.utils.payment.stripe_client import (
    StripeClient,
    NonexistentUser,
    RegionNotSupported,
    InvalidStripeToken,
    InvalidOperation,
)

# TODO (optional) make templates for boilerplate
def addSubscriptionHelper(token, email, plan, code):
    fractal_log(
        function="addSubscriptionHelper",
        label=email,
        logs="Signing {} up for plan {}, with code {}, token {}".format(
            email, plan, code, str(token)
        ),
    )
    client = StripeClient(current_app.config["STRIPE_SECRET"])
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
        except Exception as e:
            fractal_log("addSubscriptionCardHelper", "", str(e), level=logging.ERROR)

            status = INTERNAL_SERVER_ERROR
    else:
        status = BAD_REQUEST

    return jsonify({"status": status}), status


def deleteSubscriptionHelper(email):
    client = StripeClient(current_app.config["STRIPE_SECRET"])
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
    except Exception as e:
        fractal_log("deleteSubscriptionHelper", "", str(e), level=logging.ERROR)

        return jsonify({"status": INTERNAL_SERVER_ERROR}), INTERNAL_SERVER_ERROR


def addCardHelper(email, token):
    client = StripeClient(current_app.config["STRIPE_SECRET"])

    try:
        client.add_card(email, source)
        status = SUCCESS
    except NonexistentUser:
        status = FORBIDDEN
    except InvalidStripeToken:
        status = BAD_REQUEST
    except Exception as e:
        fractal_log("addCardHelper", "", str(e), level=logging.ERROR)

        status = INTERNAL_SERVER_ERROR

    return jsonify({"status": status}), status


def deleteCardHelper(email, token):
    client = StripeClient(current_app.config["STRIPE_SECRET"])

    try:
        client.delete_card(email, token)
        status = SUCCESS
    except NonexistentUser:
        status = FORBIDDEN
    except InvalidStripeToken:
        status = BAD_REQUEST
    except Exception as e:
        fractal_log("deleteCardHelper", "", str(e), level=logging.ERROR)

        status = INTERNAL_SERVER_ERROR

    return jsonify({"status": status}), status


def retrieveHelper(email):
    client = StripeClient(current_app.config["STRIPE_SECRET"])

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
                    "creditsOutstanding": 0,
                    "account_locked": False,
                    "customer": {},
                }
            ),
        )
        PAYMENT_REQUIRED,
    except Exception as e:
        fractal_log("retrieveHelper", "", str(e), level=logging.ERROR)

        return jsonify({"status": INTERNAL_SERVER_ERROR}), INTERNAL_SERVER_ERROR
