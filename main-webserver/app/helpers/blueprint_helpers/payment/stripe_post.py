from app.helpers.utils.mail.stripe_mail import (
    chargeSuccessMail,
    creditAppliedMail,
    planChangeMail,
    trialEndedMail,
    trialEndingMail,
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
from app.helpers.utils.stripe.stripe_client import (
    StripeClient,
    NonexistentUser,
    RegionNotSupported,
    InvalidStripeToken,
)


def chargeHelper(token, email, code, plan):
    fractalLog(
        function="chargeHelper",
        label=email,
        logs="Signing {} up for plan {}, with code {}, token{}".format(email, plan, code, token),
    )
    plan = (
        UNLIMITED_PLAN_ID
        if plan == "unlimited"
        else HOURLY_PLAN_ID
        if plan == "hourly"
        else MONTHLY_PLAN_ID
    )

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
    customer = User.query.get(email)
    if not customer:
        return jsonify({"status": PAYMENT_REQUIRED}), PAYMENT_REQUIRED

    subscription = stripe.Subscription.list(customer=customer.stripe_customer_id)["data"]

    if len(subscription) == 0:
        return (
            jsonify({"status": NOT_ACCEPTABLE, "error": "Customer does not have a subscription!"}),
            NOT_ACCEPTABLE,
        )

    try:
        payload = stripe.Subscription.delete(subscription[0]["id"])
        fractalLog(
            function="cancelStripeHelper",
            label=email,
            logs="Cancel stripe subscription for {}".format(email),
        )
    except Exception as e:
        fractalLog(function="cancelStripeHelper", label=email, logs=str(e), level=logging.ERROR)
        pass
    return jsonify({"status": SUCCESS}), SUCCESS


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
    customer = User.query.get(email)
    if not customer:
        return (jsonify({"status": "Customer with this email does not exist!"}), BAD_REQUEST)

    customer_id = None
    if customer.stripe_customer_id:
        customer_id = customer.stripe_customer_id
    else:
        return (jsonify({"status": "Customer does not have a Stripe ID!"}), BAD_REQUEST)

    PLAN_ID = None

    # TODO: Set PLAN_ID based on productName

    # if productName == ...:
    #     PLAN_ID = ...
    # elif ...
    #     ...

    if PLAN_ID is None:
        return (jsonify({"status": "Invalid product"}), BAD_REQUEST)

    subscription = stripe.Subscription.list(customer=customer_id)["data"][0]

    subscriptionItem = None
    for item in subscription["items"]["data"]:
        if item["plan"]["id"] == PLAN_ID:
            subscriptionItem = stripe.SubscriptionItem.retrieve(item["id"])

    if subscriptionItem is None:
        stripe.SubscriptionItem.create(subscription=subscription["id"], plan=PLAN_ID)
    else:
        stripe.SubscriptionItem.modify(
            subscriptionItem["id"], quantity=subscriptionItem["quantity"] + 1
        )

    return (jsonify({"status": "Product added to subscription successfully"}), SUCCESS)


def removeProductHelper(email, productName):
    """Removes a product from the customer

    Args:
        email (str): The email of the customer
        productName (str): Name of the product

    Returns:
        http response
    """
    customer = User.query.get(email)
    if not customer:
        return (jsonify({"status": "Customer with this email does not exist!"}), BAD_REQUEST)

    customer_id = None
    if customer.stripe_customer_id:
        customer_id = customer.stripe_customer_id
    else:
        return (jsonify({"status": "Customer does not have a Stripe ID!"}), BAD_REQUEST)

    PLAN_ID = None

    # TODO: Set PLAN_ID based on productName

    # if productName == ...:
    #     PLAN_ID = ...
    # elif ...
    #     ...

    if PLAN_ID is None:
        return (jsonify({"status": "Invalid product"}), BAD_REQUEST)

    subscription = stripe.Subscription.list(customer=customer_id)["data"][0]
    subscriptionItem = None
    for item in subscription["items"]["data"]:
        if item["plan"]["id"] == PLAN_ID:
            subscriptionItem = stripe.SubscriptionItem.retrieve(item["id"])

    if subscriptionItem is None:
        return (jsonify({"status": "Product already not in subscription"}), SUCCESS)
    else:
        if subscriptionItem["quantity"] == 1:
            stripe.SubscriptionItem.delete(subscriptionItem["id"])
        else:
            stripe.SubscriptionItem.modify(
                subscriptionItem["id"], quantity=subscriptionItem["quantity"] - 1
            )

    return (jsonify({"status": "Product removed from subscription successfully"}), SUCCESS)


def addCardHelper(custId, sourceId):
    stripe.Customer.create_source(custId, source=sourceId)

    return (jsonify({"status": "Card updated for customer successfully"}), SUCCESS)


def deleteCardHelper(custId, cardId):
    stripe.Customer.delete_source(custId, cardId)

    return (jsonify({"status": "Card removed for customer successfully"}), SUCCESS)


def webhookHelper(event):
    # Handle the event
    if event.type == "charge.failed":  # https://stripe.com/docs/api/charges
        fractalLog(
            function="webhookHelper",
            label="Stripe",
            logs="Charge failed webhook received from stripe",
        )

        # TODO: Handle failed charge.
    elif event.type == "charge.succeeded":
        fractalLog(
            function="webhookHelper",
            label="Stripe",
            logs="Charge succeeded webhook received from stripe",
        )
        custId = event.data.object.customer
        customer = User.query.filter_by(stripe_customer_id=custId).first()
        if customer:
            chargeSuccessMail(customer.user_id, custId)

    elif event.type == "customer.subscription.trial_will_end":
        fractalLog(
            function="webhookHelper",
            label="Stripe",
            logs="Charge ending webhook received from stripe",
        )
        custId = event.data.object.customer
        customer = User.query.filter_by(stripe_customer_id=custId).first()
        status = event.data.object.status
        if customer:
            if status == "trialing":
                trialEndingMail(customer.user_id)
            else:
                trialEndedMail(customer.user_id)
    else:
        return jsonify({}), NOT_FOUND

    return jsonify({"status": SUCCESS}), SUCCESS


def updateHelper(username, new_plan_type):
    new_plan_id = None
    if new_plan_type == "Hourly":
        new_plan_id = HOURLY_PLAN_ID
    elif new_plan_type == "Monthly":
        new_plan_id = MONTHLY_PLAN_ID
    elif new_plan_type == "Unlimited":
        new_plan_id = UNLIMITED_PLAN_ID
    else:
        return (jsonify({"status": NOT_ACCEPTABLE, "error": "Invalid plan type"}), NOT_ACCEPTABLE)

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


def referralHelper(code, username):
    code_username = None

    metadata = User.query.filter_by(referral_code=code).first()
    if metadata:
        code_username = metadata.user_id
    if username == code_username:
        return jsonify({"status": SUCCESS, "verified": False}), SUCCESS
    verified = code_username is not None
    return jsonify({"status": SUCCESS, "verified": verified}), SUCCESS
