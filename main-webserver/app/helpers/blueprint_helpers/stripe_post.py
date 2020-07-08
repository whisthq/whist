from app import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *

stripe.api_key = os.getenv("STRIPE_SECRET")
customer_id = ""
subscription_id = ""

def chargeHelper(token, email, code, plan):
    fractalLog(
        function="chargeHelper",
        label="Charge",
        logs ="Signing {} up for plan {}, with code {}, token{}".format(
            email, plan, code, token
        )
    )

    PLAN_ID = os.getenv("MONTHLY_PLAN_ID")
    if plan == "unlimited":
        PLAN_ID = os.getenv("UNLIMITED_PLAN_ID")
    elif plan == "hourly":
        PLAN_ID = os.getenv("HOURLY_PLAN_ID")

    trial_end = 0
    customer_exists = False

    customer = fractalSQLSelect("customers", {"username":email})
    if customer:
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
        credits = fractalSQLSelect("users", {"username":email})["credits_outstanding"]

        metadata = fractalSQLSelect("users", {"code":code})
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
            fractalSQLUpdate(table_name="users", conditional_params={"username": email}, new_params={"credits_outstanding": 0})
            subscription_id = new_subscription["id"]
    except Exception as e:
        return jsonify({"status": 402, "error": str(e)}), 402

    try:
        if customer_exists:
            #TODO
        else:
            fractalSQLInsert("customers", {email, customer_id, subscription_id, "", trial_end, True})
            fractalLog(
                function="chargeHelper",
                label="Charge",
                logs="Customer inserted successful"
            )
        
    except:
        fractalLog(
            function="chargeHelper",
            label="Charge",
            logs="Customer inserted unsuccessful",
            level=logging.ERROR
        )
        return jsonify({"status": 409}), 409

    return jsonify({"status": 200}), 200