from app.helpers.utils.mail.stripe_mail import (
    creditAppliedMail,
    planChangeMail,
)  # TODO REMOVE THIS ABOVE

from flask import jsonify

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


def chargeHelper(token, email, code, plan):
    fractalLog(
        function="chargeHelper",
        label=email,
        logs="Signing {} up for plan {}, with code {}, token{}".format(email, plan, code, token),
    )
    # TODO GET PLAN IDS

    client = StripeClient(STRIPE_SECRET)
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

    return jsonify({"status": status}), status


def retrieveStripeHelper(email):
    client = StripeClient(STRIPE_SECRET)
    try:
        info = client.get_stripe_info(email)
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


def cancelStripeHelper(email):
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
        fractalLog(function="cancelStripeHelper", label=email, logs=str(e), level=logging.ERROR)
        return jsonify({"status": INTERNAL_SERVER_ERROR}), INTERNAL_SERVER_ERROR


# TODO this is probably ok here so we can keep it but ...
def discountHelper(code):
    metadata = User.query.filter_by(referral_code=code).first()

    if not metadata:
        fractalLog(
            function="discountHelper",
            label="None",
            logs="No user for code {}".format(code),
            level=logging.ERROR,
        )
        return jsonify({"status": NOT_FOUND}), NOT_FOUND
    else:
        creditsOutstanding = metadata.credits_outstanding
        email = metadata.user_id

        metadata.credits_outstanding = creditsOutstanding + 1
        db.session.commit()

        fractalLog(
            function="discountHelper",
            label=email,
            logs="Applied discount and updated credits outstanding",
        )

        creditAppliedMail(email)

        return jsonify({"status": SUCCESS}), SUCCESS


def addProductHelper(email, productName):
    """Adds a product to the customer

    Args:
        email (str): The email of the customer
        productName (str): Name of the product

    Returns:
        http response
    """
    # TODO
    return (jsonify({"status": "Product added to subscription successfully"}), SUCCESS)


def removeProductHelper(email, productName):
    """Removes a product from the customer

    Args:
        email (str): The email of the customer
        productName (str): Name of the product

    Returns:
        http response
    """
    # TODO
    return (jsonify({"status": "Product removed from subscription successfully"}), SUCCESS)


# TODO
def updateHelper(username, new_plan_type):
    new_plan_id = None
    # TODO GET PLAN ID

    customer = User.query.get(username)
    if not customer:
        return (
            jsonify({"status": NOT_ACCEPTABLE, "error": "No user with such email exists"}),
            NOT_ACCEPTABLE,
        )

    if not customer.stripe_customer_id:
        return (
            jsonify({"status": NOT_ACCEPTABLE, "error": "User is not on stripe"}),
            NOT_ACCEPTABLE,
        )

    subscriptions = stripe.Subscription.list(customer=customer.stripe_customer_id)["data"]
    if not subscriptions:
        return (
            jsonify({"status": NOT_ACCEPTABLE, "error": "User is not on any plan"}),
            NOT_ACCEPTABLE,
        )

    stripe.SubscriptionItem.modify(subscriptions[0]["items"]["data"][0]["id"], plan=new_plan_id)
    planChangeMail(username, new_plan_type)
    return jsonify({"status": SUCCESS}), SUCCESS
