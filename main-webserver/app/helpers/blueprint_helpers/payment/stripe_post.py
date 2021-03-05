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
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.payment.stripe_client import (
    StripeClient,
    NonexistentUser,
    NonexistentStripeCustomer,
    RegionNotSupported,
    InvalidStripeToken,
    InvalidOperation,
)

# TODO (optional) make templates for boilerplate
def addSubscriptionHelper(email, plan):
    fractal_logger.info("Signing {} up for plan {}".format(email, plan), extra={"label": email})

    client = StripeClient(current_app.config["STRIPE_SECRET"])
    plans = client.get_prices()

    def reduce_plan(acc, pl):
        return acc if pl[0] != plan else pl[1]

    price = reduce(reduce_plan, plans, None)

    fractal_logger.info(f"Subscribing to {price}", extra={"label": email})

    if price:
        try:
            client.create_subscription(email, price)
            status = SUCCESS
        except NonexistentUser:
            fractal_logger.error("Non existent user", extra={"label": email})
            status = FORBIDDEN
        except NonexistentStripeCustomer:
            fractal_logger.error("Non existent Stripe customer", extra={"label": email})
            status = FORBIDDEN
        except RegionNotSupported:
            fractal_logger.error("Region not supported", extra={"label": email})
            status = BAD_REQUEST
        except InvalidStripeToken:
            fractal_logger.error("Invalid Stripe token", extra={"label": email})
            status = BAD_REQUEST
        except Exception as e:
            fractal_logger.error("", exc_info=True)
            status = INTERNAL_SERVER_ERROR
    else:
        fractal_logger.error("No plan was found", extra={"label": email})
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
        fractal_logger.error("", exc_info=True)

        return jsonify({"status": INTERNAL_SERVER_ERROR}), INTERNAL_SERVER_ERROR


def addCardHelper(email, source):
    client = StripeClient(current_app.config["STRIPE_SECRET"])

    try:
        client.add_card(email, source)
        status = SUCCESS
    except NonexistentUser:
        status = FORBIDDEN
    except InvalidStripeToken:
        status = BAD_REQUEST
    except Exception as e:
        fractal_logger.error("", exc_info=True)
        status = INTERNAL_SERVER_ERROR

    return jsonify({"status": status}), status


def deleteCardHelper(email, source):
    client = StripeClient(current_app.config["STRIPE_SECRET"])

    try:
        client.delete_card(email, source)
        status = SUCCESS
    except NonexistentUser:
        status = FORBIDDEN
    except InvalidStripeToken:
        status = BAD_REQUEST
    except Exception as e:
        fractal_logger.error("", exc_info=True)
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
                    "subscription": None,
                    "source": None,
                    "account_locked": False,
                    "customer": None,
                }
            ),
            PAYMENT_REQUIRED,
        )
    except Exception as e:
        fractal_logger.error("", exc_info=True)
        return jsonify({"status": INTERNAL_SERVER_ERROR}), INTERNAL_SERVER_ERROR
