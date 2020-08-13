from app import *
from app.helpers.utils.mail.stripe_mail import *
from pyzipcode import ZipCodeDatabase
from app.constants.states import *

stripe.api_key = os.getenv("STRIPE_SECRET")
zcdb = ZipCodeDatabase()

import logging


def chargeHelper(token, email, code, plan):
    fractalLog(
        function="chargeHelper",
        label=email,
        logs="Signing {} up for plan {}, with code {}, token{}".format(
            email, plan, code, token
        ),
    )

    PLAN_ID = os.getenv("MONTHLY_PLAN_ID")
    if plan == "unlimited":
        PLAN_ID = os.getenv("UNLIMITED_PLAN_ID")
    elif plan == "hourly":
        PLAN_ID = os.getenv("HOURLY_PLAN_ID")

    trial_end = 0
    customer_exists = False
    customer_id = ""

    output = fractalSQLSelect("users", {"email": email})
    if output:
        customer = output[0]
        if customer["stripe_customer_id"]:
            customer_id = customer["stripe_customer_id"]
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
                    customer=new_customer["id"],
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
                stripe.Subscription.modify(
                    subscriptions[0]["id"], items=[{"plan": PLAN_ID}],
                )
                fractalLog(
                    function="chargeHelper",
                    label=email,
                    logs="Customer updated successful",
                )
        else:
            new_customer = stripe.Customer.create(email=email, source=token)
            customer_id = new_customer["id"]
            credits = fractalSQLSelect("users", {"email": email})[0][
                "credits_outstanding"
            ]

            referrer = fractalSQLSelect("users", {"referral_code": code})

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

            fractalSQLUpdate(
                "users",
                {"email": email},
                {"stripe_customer_id": customer_id, "credits_outstanding": 0,},
            )
            fractalLog(
                function="chargeHelper", label=email, logs="Customer added successful",
            )

    except:
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
    customerOutput = fractalSQLSelect("users", {"email": email})

    customer = None
    if customerOutput:
        customer = customerOutput[0]
    else:
        return (
            jsonify(
                {"status": PAYMENT_REQUIRED, "error": "No such user for that email!"}
            ),
            PAYMENT_REQUIRED,
        )

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
    output = fractalSQLSelect("users", {"email": email})
    if not output:
        return jsonify({"status": PAYMENT_REQUIRED}), PAYMENT_REQUIRED

    customer = output[0]
    subscription = stripe.Subscription.list(customer=customer["stripe_customer_id"])[
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
    metadata = fractalSQLSelect("users", {"referral_code": code})

    if not metadata:
        fractalLog(
            function="discountHelper",
            label="Stripe",
            logs="No user for code {}".format(code),
            level=logging.ERROR,
        )
        return jsonify({"status": NOT_FOUND}), NOT_FOUND

    creditsOutstanding = metadata[0]["credits_outstanding"]
    email = metadata["email"]
    has_subscription = False
    subscription = None
    trial_end = 0

    if metadata["stripe_customer_id"]:
        has_subscription = True
        subscription = stripe.Subscription.list(
            customer=customer["stripe_customer_id"]
        )["data"][0]

    fractalSQLUpdate(
        "users", {"email": email}, {"credits_outstanding": creditsOutstanding + 1},
    )
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
    customers = fractalSQLSelect("users", {"email": email})
    if not customers:
        return (
            jsonify({"status": "Customer with this email does not exist!"}),
            BAD_REQUEST,
        )

    customer_id = None
    if customers[0]["stripe_customer_id"]:
        customer_id = customers[0]["stripe_customer_id"]
    else:
        return (
            jsonify({"status": "Customer does not have a Stripe ID!"}),
            BAD_REQUEST,
        )

    PLAN_ID = None
    if productName == "256disk":
        PLAN_ID = os.getenv("SMALLDISK_PLAN_ID")
    elif productName == "512disk":
        PLAN_ID = os.getenv("MEDIUMDISK_PLAN_ID")

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
    customers = fractalSQLSelect("users", {"email": email})
    if not customers:
        return (
            jsonify({"status": "Customer with this email does not exist!"}),
            BAD_REQUEST,
        )

    customer_id = None
    if customers[0]["stripe_customer_id"]:
        customer_id = customers[0]["stripe_customer_id"]
    else:
        return (
            jsonify({"status": "Customer does not have a Stripe ID!"}),
            BAD_REQUEST,
        )

    PLAN_ID = None
    if productName == "256disk":
        PLAN_ID = os.getenv("SMALLDISK_PLAN_ID")
    elif productName == "512disk":
        PLAN_ID = os.getenv("MEDIUMDISK_PLAN_ID")

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
        customer = fractalSQLSelect("customers", {"id": custId})["rows"]

        if customer:
            chargeFailedMail(customer["username"], custId)

            # Schedule disk deletion in 7 days
            expiry = (dt.today() + timedelta(days=7)).strftime("%m/%d/%Y, %H:%M")
            fractalSQLUpdate(
                "disks",
                {"username": customer["username"]},
                {"state": "TO_BE_DELETED", "delete_date": expiry},
            )

    elif event.type == "charge.succeeded":
        fractalLog(
            function="webhookHelper",
            label="Stripe",
            logs="Charge succeeded webhook received from stripe",
        )
        custId = event.data.object.customer
        customer = fractalSQLSelect("customers", {"id": custId})["rows"]
        if customer:
            chargeSuccessMail(customer["username"], custId)

    elif event.type == "customer.subscription.trial_will_end":
        fractalLog(
            function="webhookHelper",
            label="Stripe",
            logs="Charge ending webhook received from stripe",
        )
        custId = event.data.object.customer
        customer = fractalSQLSelect("customers", {"id": custId})["rows"]
        status = event.data.object.status
        if customer:
            if status == "trialing":
                trialEndingMail(customer["username"])
            else:
                trialEndedMail(customer["username"])
    else:
        return jsonify({}), NOT_FOUND

    return jsonify({"status": SUCCESS}), SUCCESS


def updateHelper(username, new_plan_type):
    new_plan_id = None
    if new_plan_type == "Hourly":
        new_plan_id = os.getenv("HOURLY_PLAN_ID")
    elif new_plan_type == "Monthly":
        new_plan_id = os.getenv("MONTHLY_PLAN_ID")
    elif new_plan_type == "Unlimited":
        new_plan_id = os.getenv("UNLIMITED_PLAN_ID")
    else:
        return (
            jsonify({"status": NOT_ACCEPTABLE, "error": "Invalid plan type"}),
            NOT_ACCEPTABLE,
        )

    customer = fractalSQLSelect("users", {"email": username})
    if not customer:
        return (
            jsonify(
                {"status": NOT_ACCEPTABLE, "error": "No user with such email exists"}
            ),
            NOT_ACCEPTABLE,
        )

    customer = customer[0]
    if not customer["stripe_customer_id"]:
        return (
            jsonify({"status": NOT_ACCEPTABLE, "error": "User is not on stripe"}),
            NOT_ACCEPTABLE,
        )

    subscriptions = stripe.Subscription.list(customer=customer["stripe_customer_id"])[
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

    metadata = fractalSQLSelect("users", {"code": code})
    if metadata:
        code_username = metadata[0]["email"]
    if username == code_username:
        return jsonify({"status": SUCCESS, "verified": False}), SUCCESS
    verified = code_username is not None
    return jsonify({"status": SUCCESS, "verified": verified}), SUCCESS
