from app.helpers.customers import *
from app.helpers.users import *
from app.helpers.disks import *
from app.helpers.mail import *
from app.logger import *

stripe_bp = Blueprint("stripe_bp", __name__)

# STRIPE endpoint


@stripe_bp.route("/stripe/<action>", methods=["POST"])
@generateID
@logRequestInfo
def payment(action, **kwargs):
    stripe.api_key = os.getenv("STRIPE_SECRET")
    customer_id = ""
    subscription_id = ""

    # Adds a subscription to the customer
    if action == "charge":
        body = request.get_json()

        token = body["token"]
        email = body["email"]
        code = body["code"]
        plan = body["plan"]
        sendInfo(
            kwargs["ID"],
            "Signing {} up for plan {}, with code {}, token{}".format(
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

        customers = fetchCustomers()
        for customer in customers:
            if email == customer["username"]:
                customer_exists = True
                trial_end = customer["trial_end"]

        try:
            new_customer = stripe.Customer.create(email=email, source=token)
            customer_id = new_customer["id"]
            credits = getUserCredits(email)

            metadata = mapCodeToUser(code)
            if metadata:
                credits += 1

            if credits == 0:
                if not customer_exists:
                    trial_end = shiftUnixByWeek(dateToUnix(getToday()), 1)
                new_subscription = stripe.Subscription.create(
                    customer=new_customer["id"],
                    items=[{"plan": PLAN_ID}],
                    trial_end=trial_end,
                    trial_from_plan=False,
                )
                subscription_id = new_subscription["id"]
            else:
                trial_end = shiftUnixByMonth(dateToUnix(getToday()), credits)
                new_subscription = stripe.Subscription.create(
                    customer=new_customer["id"],
                    items=[{"plan": PLAN_ID}],
                    trial_end=trial_end,
                    trial_from_plan=False,
                )
                changeUserCredits(email, 0)
                subscription_id = new_subscription["id"]
        except Exception as e:
            return jsonify({"status": 402, "error": str(e)}), 402

        try:
            insertCustomer(
                email, customer_id, subscription_id, "", trial_end, True, kwargs["ID"]
            )
            sendInfo(kwargs["ID"], "Customer inserted successful")
        except:
            sendInfo(kwargs["ID"], "Customer inserted unsuccessful")
            return jsonify({"status": 409}), 409

        return jsonify({"status": 200}), 200

    # Retrieves the stripe subscription of the customer
    elif action == "retrieve":
        body = request.get_json()
        email = body["email"]
        sendInfo(
            kwargs["ID"], "Retrieving subscriptions for {}".format(email),
        )

        credits = getUserCredits(email)
        customers = fetchCustomers()
        for customer in customers:
            if email == customer["username"]:
                subscription = customer["subscription"]
                try:
                    payload = stripe.Subscription.retrieve(subscription)
                    updateTrialEnd(payload["id"], payload["trial_end"])
                    account_locked = not customer["paid"] and not payload["trial_end"]
                    return (
                        jsonify(
                            {
                                "status": 200,
                                "subscription": payload,
                                "creditsOutstanding": credits,
                                "account_locked": account_locked,
                                "customer": customer,
                            }
                        ),
                        200,
                    )
                except Exception as e:
                    account_locked = not customer["paid"] and customer[
                        "trial_end"
                    ] < dateToUnix(getToday())
                    return (
                        jsonify(
                            {
                                "status": 402,
                                "subscription": {},
                                "creditsOutstanding": credits,
                                "account_locked": account_locked,
                                "customer": customer,
                            }
                        ),
                        402,
                    )

        return (
            jsonify(
                {
                    "status": 402,
                    "subscription": {},
                    "creditsOutstanding": credits,
                    "account_locked": False,
                    "customer": {},
                }
            ),
            402,
        )

    # Cancel a stripe subscription
    elif action == "cancel":
        body = request.get_json()

        email = body["email"]
        customers = fetchCustomers()
        for customer in customers:
            if email == customer["username"]:
                subscription = customer["subscription"]
                try:
                    payload = stripe.Subscription.delete(subscription)
                    sendInfo(
                        kwargs["ID"], "Cancel stripe subscription for {}".format(email)
                    )
                except Exception as e:
                    sendError(kwargs["ID"], e)
                    pass
                deleteCustomer(email)
                return jsonify({"status": 200}), 200
        return jsonify({"status": 400}), 400

    # Changes a stripe subscription
    elif action == "changeSubscription":
        body = request.get_json()

        email = body["email"]
        newPlan = body["plan"]
        customers = fetchCustomers()
        for customer in customers:
            if email == customer["username"]:
                subscriptionId = customer["subscription"]
                try:
                    subscription = stripe.Subscription.retrieve(subscriptionId)
                    payload = stripe.Subscription.modify(
                        subscription.id,
                        cancel_at_period_end=False,
                        proration_behavior="create_prorations",
                        items=[
                            {
                                "id": subscription["items"]["data"][0].id,
                                "price": "price_CBb6IXqvTLXp3f",
                            }
                        ],
                    )
                    sendInfo(
                        kwargs["ID"],
                        "Subscription changed to {} for account {}".format(
                            newPlan, email
                        ),
                    )
                except Exception as e:
                    sendError(kwargs["ID"], e)
                    pass
                deleteCustomer(email)
                return jsonify({"status": 200}), 200
        return jsonify({"status": 400}), 400

    elif action == "discount":
        body = request.get_json()

        code = body["code"]
        metadata = mapCodeToUser(code)

        if not metadata:
            sendError(kwargs["ID"], "No user for code {}".format(code))
            return jsonify({"status": 404}), 404

        creditsOutstanding = metadata["credits_outstanding"]
        email = metadata["username"]
        has_subscription = False
        subscription_id = None
        trial_end = 0

        customers = fetchCustomers()
        for customer in customers:
            if email == customer["username"] and customer["subscription"]:
                has_subscription = True
                subscription_id = customer["subscription"]
        print(has_subscription)
        if has_subscription:
            new_subscription = stripe.Subscription.retrieve(subscription_id)
            print(new_subscription)
            if new_subscription["trial_end"]:
                if new_subscription["trial_end"] <= dateToUnix(getToday()):
                    trial_end = shiftUnixByMonth(dateToUnix(getToday()), 1)
                    modified_subscription = stripe.Subscription.modify(
                        new_subscription["id"],
                        trial_end=trial_end,
                        trial_from_plan=False,
                    )
                    updateTrialEnd(new_subscription["id"], trial_end)
                else:
                    trial_end = shiftUnixByMonth(new_subscription["trial_end"], 1)
                    modified_subscription = stripe.Subscription.modify(
                        new_subscription["id"],
                        trial_end=trial_end,
                        trial_from_plan=False,
                    )
                    updateTrialEnd(new_subscription["id"], trial_end)
            else:
                trial_end = shiftUnixByMonth(dateToUnix(getToday()), 1)
                modified_subscription = stripe.Subscription.modify(
                    new_subscription["id"], trial_end=trial_end, trial_from_plan=False
                )
                updateTrialEnd(new_subscription["id"], trial_end)
            sendInfo(kwargs["ID"], "Applied discount and updated trial end")
        else:
            changeUserCredits(email, creditsOutstanding + 1)
            sendInfo(kwargs["ID"], "Applied discount and updated credits outstanding")

        creditAppliedMail(email, kwargs["ID"])

        return jsonify({"status": 200}), 200

    # Inserts a customer to the table
    elif action == "insert":
        body = request.get_json()
        trial_end = shiftUnixByWeek(dateToUnix(getToday()), 1)
        email = body["email"]
        credits = getUserCredits(email)
        if credits > 0:
            trial_end = shiftUnixByMonth(dateToUnix(getToday()), credits)
            changeUserCredits(email, 0)
        insertCustomer(
            email, None, None, body["location"], trial_end, False, kwargs["ID"]
        )
        return jsonify({"status": 200}), 200

    # Endpoint for stripe webhooks
    elif action == "hooks":
        body = request.get_data()
        sigHeader = request.headers["Stripe-Signature"]
        print(sigHeader)
        endpointSecret = "whsec_dlppdwofxgW17XsboVetpX9CvGMq9geQ"
        event = None

        try:
            event = stripe.Webhook.construct_event(body, sigHeader, endpointSecret)
        except ValueError as e:
            # Invalid payload
            return jsonify({"status": "Invalid payload"}), 400
        except stripe.error.SignatureVerificationError as e:
            # Invalid signature
            return jsonify({"status": "Invalid signature"}), 400

        # Handle the event
        if event.type == "charge.failed":  # https://stripe.com/docs/api/charges
            sendInfo(kwargs["ID"], "Charge failed webhook received from stripe")
            custId = event.data.object.customer
            customer = fetchCustomerById(custId)

            if customer:
                chargeFailedMail(user, custId, kwargs["ID"])

                # Schedule disk deletion in 7 days
                disks = fetchUserDisks(customer["username"])
                expiry = datetime.datetime.today() + timedelta(days=7)
                for disk in disks:
                    scheduleDiskDelete(disk["disk_name"], expiry)

        elif event.type == "charge.succeeded":
            sendInfo(kwargs["ID"], "Charge succeeded webhook received from stripe")
            custId = event.data.object.customer
            customer = fetchCustomerById(custId)
            if customer:
                message = SendGridMail(
                    from_email="noreply@fractalcomputers.com",
                    to_emails=["support@fractalcomputers.com"],
                    subject="Payment recieved from " + customer["username"],
                    html_content="<div>The charge has succeeded for account "
                    + custId
                    + "</div>",
                )
                try:
                    sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
                    response = sg.send(message)
                    sendInfo(kwargs["ID"], "Sent charge success email to support")
                except Exception as e:
                    print(e.message)

        elif event.type == "customer.subscription.trial_will_end":
            sendInfo(kwargs["ID"], "Trial ending webhook received from stripe")
            custId = event.data.object.customer
            customer = fetchCustomerById(custId)
            status = event.data.object.status
            if customer:
                if status == "trialing":
                    trialEndingMail(customer["username"], kwargs["ID"])
                else:
                    trialEndedMail(customer["username"], kwargs["ID"])
        else:
            return jsonify({}), 400

        return jsonify({"status": 200}), 200
    elif action == "update" and request.method == "POST":
        # When a customer requests to change their plan type
        body = request.get_json()

        username = body["username"]
        new_plan_type = body["plan"]
        new_plan_id = None
        if new_plan_type == "Hourly":
            new_plan_id = os.getenv("HOURLY_PLAN_ID")
        elif new_plan_type == "Monthly":
            new_plan_id = os.getenv("MONTHLY_PLAN_ID")
        elif new_plan_type == "Unlimited":
            new_plan_id = os.getenv("UNLIMITED_PLAN_ID")
        else:
            return jsonify({"status": 404, "error": "Invalid plan type"}), 404

        customer = fetchCustomer(username)
        if customer:
            old_subscription = customer["subscription"]
            subscription = stripe.Subscription.retrieve(old_subscription)
            if subscription:
                subscription_id = subscription["items"]["data"][0].id
                stripe.SubscriptionItem.modify(subscription_id, plan=new_plan_id)
                planChangeMail(username, new_plan_type, kwargs["ID"])
                return jsonify({"status": 200}), 200
        else:
            return jsonify({"status": 404, "error": "Invalid plan type"}), 404

    elif action == "update" and request.method == "POST":
        body = request.get_json()

        username = body["username"]
        new_plan_type = body["plan"]
        new_plan_id = None
        if new_plan_type == "Hourly":
            new_plan_id = os.getenv("HOURLY_PLAN_ID")
        elif new_plan_type == "Monthly":
            new_plan_id = os.getenv("MONTHLY_PLAN_ID")
        elif new_plan_type == "Unlimited":
            new_plan_id = os.getenv("UNLIMITED_PLAN_ID")
        else:
            return jsonify({"status": 404, "error": "Invalid plan type"}), 404

        customer = fetchCustomer(username)
        if customer:
            old_subscription = customer["subscription"]
            subscription = stripe.Subscription.retrieve(old_subscription)
            if subscription:
                subscription_id = subscription["items"]["data"][0].id
                stripe.SubscriptionItem.modify(subscription_id, plan=new_plan_id)
                return jsonify({"status": 200}), 200
        else:
            return jsonify({"status": 404, "error": "Invalid plan type"}), 404


# REFERRAL endpoint


@stripe_bp.route("/referral/<action>", methods=["POST"])
@jwt_required
@generateID
@logRequestInfo
def referral(action, **kwargs):
    if action == "validate":
        body = request.get_json()
        code = body["code"]
        code_username = None
        username = body["username"]

        metadata = mapCodeToUser(code)
        if metadata:
            code_username = metadata["username"]
        if username == code_username:
            return jsonify({"status": 200, "verified": False}), 200
        codes = fetchCodes()
        verified = code in codes
        return jsonify({"status": 200, "verified": verified}), 200
