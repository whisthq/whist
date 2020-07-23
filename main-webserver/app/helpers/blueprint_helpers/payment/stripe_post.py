from app import *
from app.helpers.utils.mail.stripe_mail import *

stripe.api_key = os.getenv("STRIPE_SECRET")
customer_id = ""
subscription_id = ""


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

    output = fractalSQLSelect("customers", {"username": email})["rows"]
    if output:
        customer = output[0]
        customer_exists = True
        if customer["trial_end"]:
            trial_end = max(
                customer["trial_end"], round((dt.now() + timedelta(days=1)).timestamp())
            )
        else:
            trial_end = round((dt.now() + timedelta(days=1)).timestamp())

    try:
        new_customer = stripe.Customer.create(email=email, source=token)
        customer_id = new_customer["id"]
        credits = fractalSQLSelect("users", {"username": email})["rows"][0][
            "credits_outstanding"
        ]

        metadata = fractalSQLSelect("users", {"code": code})["rows"]
        if metadata:
            credits += 1

        if credits == 0:
            if not customer_exists:
                trial_end = round((dt.now() + relativedelta(weeks=1)).timestamp())
            new_subscription = stripe.Subscription.create(
                customer=new_customer["id"],
                items=[{"plan": PLAN_ID}],
                trial_end=trial_end,
                trial_from_plan=False,
            )
            subscription_id = new_subscription["id"]
        else:
            trial_end = round((dt.now() + relativedelta(months=credits)).timestamp())
            new_subscription = stripe.Subscription.create(
                customer=new_customer["id"],
                items=[{"plan": PLAN_ID}],
                trial_end=trial_end,
                trial_from_plan=False,
            )
            fractalSQLUpdate(
                table_name="users",
                conditional_params={"username": email},
                new_params={"credits_outstanding": 0},
            )
            subscription_id = new_subscription["id"]
    except Exception as e:
        fractalLog(
            function="chargeHelper",
            label=email,
            logs="Stripe charge encountered a critical error: {error}".format(
                error=str(e)
            ),
            level=logging.CRITICAL,
        )
        return jsonify({"status": UNAUTHORIZED, "error": str(e)}), UNAUTHORIZED

    try:
        if customer_exists:
            fractalSQLUpdate(
                "customers",
                {"username": email},
                {
                    "id": customer_id,
                    "subscription": subscription_id,
                    "trial_end": trial_end,
                    "paid": True,
                },
            )
            fractalLog(
                function="chargeHelper", label=email, logs="Customer updated successful"
            )
        else:
            fractalSQLInsert(
                "customers",
                {
                    "username": email,
                    "id": customer_id,
                    "subscription": subscription_id,
                    "location": "",
                    "trial_end": trial_end,
                    "paid": True,
                    "created": dt.now(datetime.timezone.utc).timestamp(),
                },
            )
            fractalLog(
                function="chargeHelper",
                label=email,
                logs="Customer inserted successful",
            )

    except:
        fractalLog(
            function="chargeHelper",
            label=email,
            logs="Customer inserted unsuccessful",
            level=logging.ERROR,
        )
        return jsonify({"status": CONFLICT}), CONFLICT

    return jsonify({"status": SUCCESS}), SUCCESS


def retrieveStripeHelper(email):
    fractalLog(
        function="retrieveStripeHelper",
        label="Stripe",
        logs="Retrieving subscriptions for {}".format(email),
    )

    customerOutput = fractalSQLSelect("customers", {"username": email})
    customer = None
    if customerOutput["rows"]:
        customer = customerOutput["rows"][0]
    creditsOutput = fractalSQLSelect("users", {"username": email})
    if creditsOutput["rows"]:
        credits = creditsOutput["rows"][0]["credits_outstanding"]
    else:
        credits = 0

    if customer:
        subscription = customer["subscription"]
        try:
            payload = stripe.Subscription.retrieve(subscription)
            fractalSQLUpdate(
                "customers",
                {"subscription": payload["id"]},
                {"trial_end": payload["trial_end"]},
            )
            account_locked = not customer["paid"] and not payload["trial_end"]
            return (
                jsonify(
                    {
                        "status": SUCCESS,
                        "subscription": payload,
                        "creditsOutstanding": credits,
                        "account_locked": account_locked,
                        "customer": customer,
                    }
                ),
                SUCCESS,
            )
        except Exception as e:
            account_locked = not customer["paid"] and customer[
                "trial_end"
            ] < dateToUnix(getToday())

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
                        "creditsOutstanding": credits,
                        "account_locked": account_locked,
                        "customer": customer,
                    }
                ),
                PAYMENT_REQUIRED,
            )

    return (
        jsonify(
            {
                "status": PAYMENT_REQUIRED,
                "subscription": {},
                "creditsOutstanding": credits,
                "account_locked": False,
                "customer": {},
            }
        ),
        PAYMENT_REQUIRED,
    )


def cancelStripeHelper(email):
    output = fractalSQLSelect("customers", {"username": email})["rows"]
    if output:
        customer = output[0]
        subscription = customer["subscription"]
        try:
            payload = stripe.Subscription.delete(subscription)
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
        fractalSQLDelete("customers", {"username": email})
        return jsonify({"status": SUCCESS}), SUCCESS
    return jsonify({"status": NOT_FOUND}), NOT_FOUND


def discountHelper(code):
    metadata = fractalSQLSelect("users", {"code": code})["rows"]

    if not metadata:
        fractalLog(
            function="discountHelper",
            label="Stripe",
            logs="No user for code {}".format(code),
            level=logging.ERROR,
        )
        return jsonify({"status": NOT_FOUND}), NOT_FOUND

    creditsOutstanding = metadata["credits_outstanding"]
    email = metadata["username"]
    has_subscription = False
    subscription_id = None
    trial_end = 0

    customer = fractalSQLSelect("customers", {"username": email})["rows"]
    if customer and customer["subscription"]:
        has_subscription = True
        subscription_id = customer["subscription"]

    if has_subscription:
        new_subscription = stripe.Subscription.retrieve(subscription_id)
        if new_subscription["trial_end"]:
            if new_subscription["trial_end"] <= round(dt.now().timestamp()):
                trial_end = round((dt.now() + relativedelta(months=1)).timestamp())
                modified_subscription = stripe.Subscription.modify(
                    new_subscription["id"], trial_end=trial_end, trial_from_plan=False,
                )
                fractalSQLUpdate(
                    "customers",
                    {"subscription": new_subscription["id"]},
                    {"trial_end": trial_end},
                )
            else:
                trial_end = round(
                    (
                        new_subscription["trial_end"] + relativedelta(months=1)
                    ).timestamp()
                )
                modified_subscription = stripe.Subscription.modify(
                    new_subscription["id"], trial_end=trial_end, trial_from_plan=False,
                )
                fractalSQLUpdate(
                    "customers",
                    {"subscription": new_subscription["id"]},
                    {"trial_end": trial_end},
                )
        else:
            trial_end = round((dt.now() + relativedelta(months=1)).timestamp())
            modified_subscription = stripe.Subscription.modify(
                new_subscription["id"], trial_end=trial_end, trial_from_plan=False
            )
            fractalSQLUpdate(
                "customers",
                {"subscription": new_subscription["id"]},
                {"trial_end": trial_end},
            )
        fractalLog(
            function="discountHelper",
            label=code,
            logs="Applied discount and updated trial end",
        )
    else:
        fractalSQLUpdate(
            "users",
            {"username": email},
            {"credits_outstanding": creditsOutstanding + 1},
        )
        fractalLog(
            function="discountHelper",
            label=email,
            logs="Applied discount and updated credits outstanding",
        )

    creditAppliedMail(email)

    return jsonify({"status": SUCCESS}), SUCCESS


def insertCustomerHelper(email, location):
    trial_end = round((dt.now() + relativedelta(weeks=1)).timestamp())
    credits = 0
    user = fractalSQLSelect("users", {"username": email})["rows"]
    if user:
        credits = user["credits_outstanding"]
    if credits > 0:
        trial_end = round((dt.now() + relativedelta(months=credits)).timestamp())
        fractalSQLUpdate(
            table_name="users",
            conditional_params={"username": email},
            new_params={"credits_outstanding": 0},
        )

    fractalSQLInsert(
        "customers",
        {
            "username": email,
            "id": None,
            "subscription": None,
            "location": location,
            "trial_end": trial_end,
            "paid": False,
            "created": dt.now(datetime.timezone.utc).timestamp(),
        },
    )

    return jsonify({"status": SUCCESS}), SUCCESS


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

    customer = fractalSQLSelect("customers", {"username": username})["rows"]
    if customer:
        customer = customer[0]
        old_subscription = customer["subscription"]
        subscription = stripe.Subscription.retrieve(old_subscription)
        if subscription:
            subscription_id = subscription["items"]["data"][0].id
            stripe.SubscriptionItem.modify(subscription_id, plan=new_plan_id)
            planChangeMail(username, new_plan_type)
            return jsonify({"status": SUCCESS}), SUCCESS
    else:
        return (
            jsonify({"status": NOT_ACCEPTABLE, "error": "Invalid plan type"}),
            NOT_ACCEPTABLE,
        )


def referralHelper(code, username):
    code_username = None

    metadata = fractalSQLSelect("users", {"code": code})["rows"]
    if metadata:
        code_username = metadata["username"]
    if username == code_username:
        return jsonify({"status": SUCCESS, "verified": False}), SUCCESS
    codes = fractalSQLSelect("users", {"code": code})["rows"]
    verified = code_username is not None
    return jsonify({"status": SUCCESS, "verified": verified}), SUCCESS
