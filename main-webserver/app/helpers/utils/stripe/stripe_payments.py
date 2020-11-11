import logging

from datetime import datetime as dt
from datetime import timedelta

import stripe

from app.constants.config import STRIPE_SECRET
from app.helpers.utils.general.logs import fractalLog
from app.models import LoginHistory, User

HOURLY_PLAN_ID = "DUMMY"


def stripeChargeHourly(username):

    # Check to see if the user is a current customer
    customer = User.query.get(username)

    if not customer:
        return

    # Check to see if user is an hourly plan subscriber

    stripe.api_key = STRIPE_SECRET
    subscription_id = customer.stripe_customer_id

    try:
        payload = stripe.Subscription.retrieve(subscription_id)

        if HOURLY_PLAN_ID == payload["items"]["data"][0]["plan"]["id"]:
            user_activity = (
                LoginHistory.query.filter_by(user_id=username)
                .order_by(LoginHistory.timestamp.desc())
                .first()
            )

            if user_activity.action != "logon":
                fractalLog(
                    function="stripeChargeHourly",
                    label=str(username),
                    logs="{username} logged off and is an hourly subscriber, but no logon was found".format(
                        username=username
                    ),
                    level=logging.ERROR,
                )
            else:
                now = dt.now()
                logon = dt.strptime(user_activity["timestamp"], "%m-%d-%Y, %H:%M:%S")

                hours_used = now - logon
                if hours_used > timedelta(minutes=0):
                    amount = round(79 * (hours_used).total_seconds() / 60 / 60)

                    fractalLog(
                        function="stripeChargeHourly",
                        label=str(username),
                        logs="{username} used Fractal for {hours_used} hours and is an hourly subscriber. Charging {amount} cents".format(
                            username=username, hours_used=str(hours_used), amount=amount
                        ),
                    )
    except Exception as e:
        fractalLog(
            function="stripeChargeHourly",
            label=str(username),
            logs="Error charging {username} for hourly plan: {error}".format(
                username=username, error=str(e)
            ),
        )

    return
