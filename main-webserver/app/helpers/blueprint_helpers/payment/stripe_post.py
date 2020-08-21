from app import *
from app.helpers.utils.mail.stripe_mail import *
from pyzipcode import ZipCodeDatabase
from app.constants.states import *

from app.models.public import *
from app.models.hardware import *
from app.serializers.public import *
from app.serializers.hardware import *

stripe.api_key = STRIPE_SECRET
zcdb = ZipCodeDatabase()

import logging

user_schema = UserSchema()

def chargeHelper(token, email, code, plan):
    fractalLog(
        function="chargeHelper",
        label=email,
        logs="Signing {} up for plan {}, with code {}, token{}".format(
            email, plan, code, token
        ),
    )

    PLAN_ID = MONTHLY_PLAN_ID
    if plan == "unlimited":
        PLAN_ID = UNLIMITED_PLAN_ID
    elif plan == "hourly":
        PLAN_ID = HOURLY_PLAN_ID

    trial_end = 0
    customer_exists = False
    customer_id = ""

    customer = User.query.get(email)
    if customer:
        if customer.stripe_customer_id:
            customer_id = customer.stripe_customer_id
            customer_exists = True
        else:
            trial_end = round((dt.now() + timedelta(days=1)).timestamp())
    else:
        return (
            jsonify(
                {
                    "status": CONFLICT,
                    "error": "Cannot apply subscription to nonexistant user!",
                }
            ),
            CONFLICT,
        )

    try:
        zipCode = stripe.Token.retrieve(token)["card"]["address_zip"]
        purchaseState = None
        try:
            purchaseState = zcdb[zipCode].state
        except IndexError:
            return (
                jsonify(
                    {
                        "status": "NOT_ACCEPTABLE",
                        "error": "Zip code does not exist in the US!",
                    }
                ),
                NOT_ACCEPTABLE,
            )

        taxRates = stripe.TaxRate.list(limit=100, active=True)["data"]
        taxRate = []
        logger = logging.getLogger(__name__)
        for tax in taxRates:
            if (
                tax["jurisdiction"].strip().lower()
                == STATE_LIST[purchaseState].strip().lower()
            ):
                taxRate = [tax["id"]]

        if customer_exists:
            subscriptions = stripe.Subscription.list(customer=customer_id)["data"]
            if len(subscriptions) == 0:
                trial_end = round((dt.now() + relativedelta(weeks=1)).timestamp())
                stripe.Subscription.create(
                    customer=customer_id,
                    items=[{"plan": PLAN_ID}],
                    trial_end=trial_end,
                    trial_from_plan=False,
                    default_tax_rates=taxRate,
                )
                fractalLog(
                    function="chargeHelper",
                    label=email,
                    logs="Customer subscription created successful",
                )
            else:
                stripe.SubscriptionItem.modify(
                    subscriptions[0]["items"]["data"][0]["id"], plan=PLAN_ID
                )
                fractalLog(
                    function="chargeHelper",
                    label=email,
                    logs="Customer updated successful",
                )
        else:
            new_customer = stripe.Customer.create(email=email, source=token)
            customer_id = new_customer["id"]
            user = User.query.get(email)
            credits = user.credits_outstanding

            referrer = User.query.filter_by(referral_code=code).first()

            if referrer:
                trial_end = round((dt.now() + relativedelta(months=1)).timestamp())
                stripe.Subscription.create(
                    customer=customer_id,
                    items=[{"plan": PLAN_ID}],
                    trial_end=trial_end,
                    trial_from_plan=False,
                    default_tax_rates=taxRate,
                )

            else:
                trial_end = round((dt.now() + relativedelta(weeks=1)).timestamp())
                stripe.Subscription.create(
                    customer=new_customer["id"],
                    items=[{"plan": PLAN_ID}],
                    trial_end=trial_end,
                    trial_from_plan=False,
                    default_tax_rates=taxRate,
                )

            user.stripe_customer_id = customer_id
            user.credits_outstanding = 0
            db.session.commit()

            fractalLog(
                function="chargeHelper", label=email, logs="Customer added successful",
            )

    except Exception as e:
        track = traceback.format_exc()
        fractalLog(
            function="chargeHelper",
            label=email,
            logs="Stripe charge encountered a critical error: {error}".format(
                error=track
            ),
            level=logging.CRITICAL,
        )
        return jsonify({"status": CONFLICT, "error": str(e)}), CONFLICT

    return jsonify({"status": SUCCESS}), SUCCESS


def retrieveStripeHelper(email):
    fractalLog(
        function="retrieveStripeHelper",
        label="Stripe",
        logs="Retrieving subscriptions for {}".format(email),
    )
    customer = User.query.get(email)

    if not customer:
        return (
            jsonify(
                {"status": PAYMENT_REQUIRED, "error": "No such user for that email!"}
            ),
            PAYMENT_REQUIRED,
        )

    customer = user_schema.dump(customer)
    credits = customer["credits_outstanding"]

    try:
        payload = stripe.Customer.retrieve(customer["stripe_customer_id"])
        subscription = stripe.Subscription.list(
            customer=customer["stripe_customer_id"]
        )["data"][0]
        cards = payload["sources"]["data"]

        account_locked = subscription["trial_end"] < dateToUnix(getToday())

        return (
            jsonify(
                {
                    "status": SUCCESS,
                    "subscription": subscription,
                    "cards": cards,
                    "creditsOutstanding": credits,
                    "account_locked": account_locked,
                    "customer": customer,
                }
            ),
            SUCCESS,
        )
    except Exception as e:
        fractalLog(
            function="retrieveStripeHelper",
            label="Stripe",
            logs=str(e),
            level=logging.ERROR,
        )

        return (
            jsonify(
                {
                    "status": PAYMENT_REQUIRED,
                    "subscription": {},
                    "cards": [],
                    "creditsOutstanding": credits,
                    "account_locked": False,
                    "customer": customer,
                }
            ),
            PAYMENT_REQUIRED,
        )


def cancelStripeHelper(email):
    customer = User.query.get(email)
    if not customer:
        return jsonify({"status": PAYMENT_REQUIRED}), PAYMENT_REQUIRED

    subscription = stripe.Subscription.list(customer=customer.stripe_customer_id)[
        "data"
    ]

    if len(subscription) == 0:
        return (
            jsonify(
                {
                    "status": NOT_ACCEPTABLE,
                    "error": "Customer does not have a subscription!",
                }
            ),
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
        fractalLog(
            function="cancelStripeHelper",
            label=email,
            logs=str(e),
            level=logging.ERROR,
        )
        pass
    return jsonify({"status": SUCCESS}), SUCCESS


def discountHelper(code):
    metadata = User.query.filter_by(referral_code=code).first()

    if not metadata:
        fractalLog(
            function="discountHelper",
            label="Stripe",
            logs="No user for code {}".format(code),
            level=logging.ERROR,
        )
        return jsonify({"status": NOT_FOUND}), NOT_FOUND

    creditsOutstanding = metadata.credits_outstanding
    email = metadata.user_id
    has_subscription = False
    subscription = None
    trial_end = 0

    if metadata.stripe_customer_id:
        has_subscription = True
        subscription = stripe.Subscription.list(
            customer=customer.stripe_customer_id
        )["data"][0]

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
        return (
            jsonify({"status": "Customer with this email does not exist!"}),
            BAD_REQUEST,
        )

    customer_id = None
    if customers.stripe_customer_id:
        customer_id = customers.stripe_customer_id
    else:
        return (
            jsonify({"status": "Customer does not have a Stripe ID!"}),
            BAD_REQUEST,
        )

    PLAN_ID = None
    if productName == "256disk":
        PLAN_ID = SMALLDISK_PLAN_ID
    elif productName == "512disk":
        PLAN_ID = MEDIUMDISK_PLAN_ID

    if PLAN_ID is None:
        return (
            jsonify({"status": "Invalid product"}),
            BAD_REQUEST,
        )

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

    return (
        jsonify({"status": "Product added to subscription successfully"}),
        SUCCESS,
    )


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
        return (
            jsonify({"status": "Customer with this email does not exist!"}),
            BAD_REQUEST,
        )

    customer_id = None
    if customers.stripe_customer_id:
        customer_id = customers.stripe_customer_id
    else:
        return (
            jsonify({"status": "Customer does not have a Stripe ID!"}),
            BAD_REQUEST,
        )

    PLAN_ID = None
    if productName == "256disk":
        PLAN_ID = SMALLDISK_PLAN_ID
    elif productName == "512disk":
        PLAN_ID = MEDIUMDISK_PLAN_ID

    if PLAN_ID is None:
        return (
            jsonify({"status": "Invalid product"}),
            BAD_REQUEST,
        )

    subscription = stripe.Subscription.list(customer=customer_id)["data"][0]
    subscriptionItem = None
    for item in subscription["items"]["data"]:
        if item["plan"]["id"] == PLAN_ID:
            subscriptionItem = stripe.SubscriptionItem.retrieve(item["id"])

    if subscriptionItem is None:
        return (
            jsonify({"status": "Product already not in subscription"}),
            SUCCESS,
        )
    else:
        if subscriptionItem["quantity"] == 1:
            stripe.SubscriptionItem.delete(subscriptionItem["id"])
        else:
            stripe.SubscriptionItem.modify(
                subscriptionItem["id"], quantity=subscriptionItem["quantity"] - 1
            )

    return (
        jsonify({"status": "Product removed from subscription successfully"}),
        SUCCESS,
    )


def addCardHelper(custId, sourceId):
    stripe.Customer.create_source(
        custId, source=sourceId,
    )

    return (
        jsonify({"status": "Card updated for customer successfully"}),
        SUCCESS,
    )


def deleteCardHelper(custId, cardId):
    stripe.Customer.delete_source(
        custId, cardId,
    )

    return (
        jsonify({"status": "Card removed for customer successfully"}),
        SUCCESS,
    )


def webhookHelper(event):
    # Handle the event
    if event.type == "charge.failed":  # https://stripe.com/docs/api/charges
        fractalLog(
            function="webhookHelper",
            label="Stripe",
            logs="Charge failed webhook received from stripe",
        )
        custId = event.data.object.customer
        customer = User.query.filter_by(stripe_customer_id=custId).first()

        if customer:
            chargeFailedMail(customer.email, custId)

            # Schedule disk deletion in 7 days
            expiry = (dt.today() + timedelta(days=7)).strftime("%m/%d/%Y, %H:%M")

            disks = OSDisk.query.filter_by(user_id=customer.user_id).update({ "state": "TO_BE_DELETED", "delete_date": expiry})
            db.session.commit()

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
        return (
            jsonify({"status": NOT_ACCEPTABLE, "error": "Invalid plan type"}),
            NOT_ACCEPTABLE,
        )

    customer = User.query.get(username)
    if not customer:
        return (
            jsonify(
                {"status": NOT_ACCEPTABLE, "error": "No user with such email exists"}
            ),
            NOT_ACCEPTABLE,
        )

    if not customer.stripe_customer_id:
        return (
            jsonify({"status": NOT_ACCEPTABLE, "error": "User is not on stripe"}),
            NOT_ACCEPTABLE,
        )

    subscriptions = stripe.Subscription.list(customer=customer.stripe_customer_id)[
        "data"
    ]
    if not subscriptions:
        return (
            jsonify({"status": NOT_ACCEPTABLE, "error": "User is not on any plan"}),
            NOT_ACCEPTABLE,
        )

    stripe.SubscriptionItem.modify(
        subscriptions[0]["items"]["data"][0]["id"], plan=new_plan_id
    )
    planChangeMail(username, new_plan_type)
    return jsonify({"status": SUCCESS}), SUCCESS


def referralHelper(code, username):
    code_username = None

    metadata = User.query.filter_by(referral_code=code)
    if metadata:
        code_username = metadata.user_id
    if username == code_username:
        return jsonify({"status": SUCCESS, "verified": False}), SUCCESS
    verified = code_username is not None
    return jsonify({"status": SUCCESS, "verified": verified}), SUCCESS
