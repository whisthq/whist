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

# TODO:
# 1. send more of the emails
# 2. make a template for all this boilerplate code


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


## TODO leaving these for posterity, they might be helpful at some point
# def addProductHelper(email, productName):
#     """Adds a product to the customer
#     Args:
#         email (str): The email of the customer
#         productName (str): Name of the product
#     Returns:
#         http response
#     """
#     customer = User.query.get(email)
#     if not customer:
#         return (jsonify({"status": "Customer with this email does not exist!"}), BAD_REQUEST)

#     customer_id = None
#     if customer.stripe_customer_id:
#         customer_id = customer.stripe_customer_id
#     else:
#         return (jsonify({"status": "Customer does not have a Stripe ID!"}), BAD_REQUEST)

#     PLAN_ID = None

#     # TODO: Set PLAN_ID based on productName

#     # if productName == ...:
#     #     PLAN_ID = ...
#     # elif ...
#     #     ...

#     if PLAN_ID is None:
#         return (jsonify({"status": "Invalid product"}), BAD_REQUEST)

#     subscription = stripe.Subscription.list(customer=customer_id)["data"][0]

#     subscriptionItem = None
#     for item in subscription["items"]["data"]:
#         if item["plan"]["id"] == PLAN_ID:
#             subscriptionItem = stripe.SubscriptionItem.retrieve(item["id"])

#     if subscriptionItem is None:
#         stripe.SubscriptionItem.create(subscription=subscription["id"], plan=PLAN_ID)
#     else:
#         stripe.SubscriptionItem.modify(
#             subscriptionItem["id"], quantity=subscriptionItem["quantity"] + 1
#         )

#     return (jsonify({"status": "Product added to subscription successfully"}), SUCCESS)


# def removeProductHelper(email, productName):
#     """Removes a product from the customer
#     Args:
#         email (str): The email of the customer
#         productName (str): Name of the product
#     Returns:
#         http response
#     """
#     customer = User.query.get(email)
#     if not customer:
#         return (jsonify({"status": "Customer with this email does not exist!"}), BAD_REQUEST)

#     customer_id = None
#     if customer.stripe_customer_id:
#         customer_id = customer.stripe_customer_id
#     else:
#         return (jsonify({"status": "Customer does not have a Stripe ID!"}), BAD_REQUEST)

#     PLAN_ID = None

#     # TODO: Set PLAN_ID based on productName

#     # if productName == ...:
#     #     PLAN_ID = ...
#     # elif ...
#     #     ...

#     if PLAN_ID is None:
#         return (jsonify({"status": "Invalid product"}), BAD_REQUEST)

#     subscription = stripe.Subscription.list(customer=customer_id)["data"][0]
#     subscriptionItem = None
#     for item in subscription["items"]["data"]:
#         if item["plan"]["id"] == PLAN_ID:
#             subscriptionItem = stripe.SubscriptionItem.retrieve(item["id"])

#     if subscriptionItem is None:
#         return (jsonify({"status": "Product already not in subscription"}), SUCCESS)
#     else:
#         if subscriptionItem["quantity"] == 1:
#             stripe.SubscriptionItem.delete(subscriptionItem["id"])
#         else:
#             stripe.SubscriptionItem.modify(
#                 subscriptionItem["id"], quantity=subscriptionItem["quantity"] - 1
#             )

#     return (jsonify({"status": "Product removed from subscription successfully"}), SUCCESS)
