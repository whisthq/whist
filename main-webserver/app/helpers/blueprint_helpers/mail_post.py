from app import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *

def forgotPasswordHelper(username):
    params = {"userName": username}
    user = fractalSQLSelect("users", params)
    verified = len(user) > 0
    if verified:
        upperCase = string.ascii_uppercase
        lowerCase = string.ascii_lowercase
        numbers = "1234567890"
        c1 = "".join([random.choice(upperCase) for _ in range(0, 3)])
        c2 = "".join([random.choice(lowerCase) for _ in range(0, 9)]) + c1
        c3 = "".join([random.choice(lowerCase) for _ in range(0, 5)]) + c2
        c4 = "".join([random.choice(numbers) for _ in range(0, 4)]) + c3
        token = "".join(random.sample(c4, len(c4)))
        timeIssued = dt.now().strftime("%m-%d-%Y, %H:%M:%S")

        message = SendGridMail(
            from_email="noreply@fractalcomputers.com",
            to_emails=[username],
            subject="Reset Your Password",
            html_content=render_template(
                "on_password_forget.html",
                url=os.getenv("FRONTEND_URL"),
                token=token,
            ),
        )
        try:
            sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
            response = sg.send(message)
        except Exception as e:
            fractalLog(
                function="forgotPasswordHelper", label="ERROR", logs="Mail send failed: Error code " + e.message, level=logging.ERROR
            )
            return jsonify({"status": 500}), 500

        fractalSQLInsert("password_tokens", {
            "token": token,
            "time_issued":dt.now().strftime("%m-%d-%Y, %H:%M:%S"),
        })
        return jsonify({"verified": verified}), 200
    else:
        conn.close()
        return jsonify({"verified": verified}), 401

def resetPasswordHelper(username, password):
    """Updates the password for a user in the users SQL table

    Args:
        username (str): The user to update the password for
        password (str): The new password
    """
    pwd_token = jwt.encode({"pwd": password}, os.getenv("SECRET_KEY"))
    fractalSQLUpdate(table_name="users",
        conditional_params={"username": username},
        new_params={"password": pwd_token},)

def cancelHelper(user, feedback):
    title = "[CANCELLED PLAN + FEEDBACK] " + user + " has Just Cancelled Their Plan"

    internal_message = SendGridMail(
        from_email="noreply@fractalcomputers.com",
        to_emails=["pipitone@fractalcomputers.com", "support@fractalcomputers.com"],
        subject=title,
        html_content=feedback,
    )

    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(internal_message)
    except Exception as e:
        fractalLog(
            function="cancelHelper", label="ERROR", logs="Mail send failed: Error code " + e.message, level=logging.ERROR
        )
        return jsonify({"status": 500}), 500

    return jsonify({"status": 200}), 200

def verificationHelper(user, token):
    title = "[Fractal] Please Verify Your Email"
    url = os.getenv("FRONTEND_URL") + "/verify?" + token

    internal_message = SendGridMail(
        from_email="noreply@fractalcomputers.com",
        to_emails=user,
        subject=title,
        html_content=render_template("on_email_verification.html", url=url),
    )
    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(internal_message)
    except Exception as e:
        fractalLog(
            function="verificationHelper", label="ERROR", logs="Mail send failed: Error code " + e.message, level=logging.ERROR
        )
        return jsonify({"status": 500}), 500

    return jsonify({"status": 200}), 200

def referralHelper(user, recipients, code):
    title = "Check out Fractal"

    internal_message = SendGridMail(
        from_email=user,
        to_emails=recipients,
        subject=title,
        html_content=render_template(
            "on_referral.html", code=code, user=user
        ),
    )

    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(internal_message)
    except Exception as e:
        fractalLog(
            function="referralHelper", label="ERROR", logs="Mail send failed: Error code " + e.message, level=logging.ERROR
        )
        return jsonify({"status": 500}), 500

    return jsonify({"status": 200}), 200

def feedbackHelper(user, feedback, feedback_type):
    title = "[{}] Feedback from {}".format(feedback_type, user)

    message = SendGridMail(
        from_email="noreply@fractalcomputers.com",
        to_emails=["pipitone@fractalcomputers.com", "support@fractalcomputers.com"],
        subject=title,
        html_content="<div>" + feedback + "</div>",
    )
    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="feedbackHelper", label="ERROR", logs="Mail send failed: Error code " + e.message, level=logging.ERROR
        )
        return jsonify({"status": 500}), 500

    return jsonify({"status": 200}), 200

def trialStartHelper(user, location, code):
    message = SendGridMail(
        from_email="pipitone@fractalcomputers.com",
        to_emails=user,
        subject="Your Free Trial has Started",
        html_content=render_template(
            "on_purchase.html", location=location, code=code
        ),
    )
    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="trialStartHelper", label="ERROR", logs="Mail send failed: Error code " + e.message, level=logging.ERROR
        )
        return jsonify({"status": 500}), 500

    internal_message = SendGridMail(
        from_email="noreply@fractalcomputers.com",
        to_emails=["pipitone@fractalcomputers.com", "support@fractalcomputers.com"],
        subject="[FREE TRIAL START] A new user, " + user + ", just signed up for the free trial.",
        html_content="<div>No action needed from our part at this point.</div>",
    )
    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(internal_message)
    except Exception as e:
        fractalLog(
            function="trialStartHelper", label="ERROR", logs="Mail send failed: Error code " + e.message, level=logging.ERROR
        )
        return jsonify({"status": 500}), 500

    return jsonify({"status": 200}), 200

def signupMailHelper(user, code):
    title = "Welcome to Fractal"

    internal_message = SendGridMail(
        from_email="phil@fractalcomputers.com",
        to_emails=user,
        subject=title,
        html_content=render_template("on_signup.html", code=code),
    )

    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(internal_message)
    except Exception as e:
        fractalLog(
            function="signupMailHelper", label="ERROR", logs="Mail send failed: Error code " + e.message, level=logging.ERROR
        )
        return jsonify({"status": 500}), 500

    return jsonify({"status": 200}), 200

def computerReadyHelper(user, date, code, location):
    title = "Your Cloud PC Is Ready!"

    internal_message = SendGridMail(
        from_email="support@fractalcomputers.com",
        to_emails=user,
        subject=title,
        html_content=render_template(
            "on_cloud_pc_ready.html", date=date, code=code, location=location
        ),
    )

    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(internal_message)
    except Exception as e:
        fractalLog(
            function="computerReadyHelper", label="ERROR", logs="Mail send failed: Error code " + e.message, level=logging.ERROR
        )
        return jsonify({"status": 500}), 500

    return jsonify({"status": 200}), 200

def newsletterSubscribe(username):
    fractalSQLInsert("newsletter", {"username": username})

def newsletterUnsubscribe(username):
    fractalSQLDelete("newsletter", {"username": username})