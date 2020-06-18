from app import *


def stripeChargeHourly(username):
    
    # Check to see if the user is a current customer
    
    output = fractalSQLSelect(table_name="customers", params={"username": username})

    customer = None
    if output["success"] and output["rows"]:
        customer = output["rows"][0]
    else:
        return 

    # Check to see if user is an hourly plan subscriber
    
    stripe.api_key = os.getenv("STRIPE_SECRET")
    subscription_id = customer["subscription"]
    
    try:
        payload = stripe.Subscription.retrieve(subscription_id)

        if (os.getenv("HOURLY_PLAN_ID") == payload["items"]["data"][0]["plan"]["id"]):
            command = text(
                """
                SELECT *
                FROM login_history
                WHERE "username" = :username
                ORDER BY timestamp DESC LIMIT 1
                """
            )

            params = {"username": username}
            
            output = fractalRunSQL(command, params)
            
            
            user_activity = None 
            if output["success"] and output["rows"]:
                user_activity = output["rows"][0]
            
            if user_activity["action"] != "logon":
                fractalLog(
                    function="stripeChargeHourly",
                    label=str(username),
                    logs="{username} logged off and is an hourly subscriber, but no logon was found".format(username=username),
                    level=logging.ERROR
                )
            else:
                now = dt.now()
                logon = dt.strptime(
                    user_activity["timestamp"], "%m-%d-%Y, %H:%M:%S"
                )
                
                hours_used = now - logon 
                if hours_used > timedelta(minutes = 0):
                    amount = round(
                        79 * (hours_used).total_seconds() / 60 / 60
                    )
                
                    fractalLog(
                        function="stripeChargeHourly",
                        label=str(username),
                        logs="{username} used Fractal for {hours_used} hours and is an hourly subscriber. Charging {amount} cents".format(
                            username=username, hours_used=str(hours_used), amount=amount
                        )
                    )
    except Exception as e:
        fractalLog(
            function="stripeChargeHourly",
            label=str(username),
            logs="Error charging {username} for hourly plan: {error}".format(username=username, error=str(e))
            )
        )
        
    return