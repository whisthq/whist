from app import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *


def forgotPasswordHelper(username):
    params = {"email": username}
    user = fractalSQLSelect("users", params)
    fractalLog(
        function="forgotPasswordHelper",
        label=username,
        logs="POOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOo",
    )
    if user:
        upperCase = string.ascii_uppercase
        lowerCase = string.ascii_lowercase
        numbers = "1234567890"
        token = jwt.encode(
            {
                "sub": username,
                "exp": (dt.now() + timedelta(minutes=10))
                .replace(tzinfo=timezone.utc)
                .timestamp(),
            },
            os.getenv("SECRET_KEY"),
        )
        timeIssued = dt.now().strftime("%m-%d-%Y, %H:%M:%S")

        message = SendGridMail(
            from_email="noreply@fractalcomputers.com",
            to_emails=[username],
            subject="Reset Your Password",
            html_content=render_template(
                "on_password_forget.html", url=FRONTEND_URL, token=token,
            ),
        )
        try:
            sg = SendGridAPIClient(SENDGRID_API_KEY)
            response = sg.send(message)
        except Exception as e:
            fractalLog(
                function="forgotPasswordHelper",
                label=username,
                logs="Mail send failed: Error code " + e.message,
                level=logging.ERROR,
            )
            return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

        return jsonify({"verified": True}), SUCCESS
    else:
        return jsonify({"verified": False}), NOT_FOUND


def cancelHelper(user, feedback):
    title = "[CANCELLED PLAN + FEEDBACK] " + user + " has Just Cancelled Their Plan"

    internal_message = SendGridMail(
        from_email="noreply@fractalcomputers.com",
        to_emails=["pipitone@fractalcomputers.com", "support@fractalcomputers.com"],
        subject=title,
        html_content=feedback,
    )

    try:
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(internal_message)
    except Exception as e:
        fractalLog(
            function="cancelHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def verificationHelper(user, token):
    title = "[Fractal] Please Verify Your Email"
    url = FRONTEND_URL + "/verify?" + token
    # url = "https://localhost:3000/verify?" + token

    internal_message = SendGridMail(
        from_email="noreply@fractalcomputers.com",
        to_emails=user,
        subject=title,
        html_content=render_template("on_email_verification.html", url=url),
    )
    try:
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(internal_message)
    except Exception as e:
        fractalLog(
            function="verificationHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def referralMailHelper(user, recipients, code):
    title = "Check out Fractal"

    internal_message = SendGridMail(
        from_email=user,
        to_emails=recipients,
        subject=title,
        html_content=render_template("on_referral.html", code=code, user=user),
    )

    try:
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(internal_message)
    except Exception as e:
        fractalLog(
            function="referralMailHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def feedbackHelper(user, feedback, feedback_type):
    title = "[{}] Feedback from {}".format(feedback_type, user)

    message = SendGridMail(
        from_email="noreply@fractalcomputers.com",
        to_emails=["pipitone@fractalcomputers.com", "support@fractalcomputers.com"],
        subject=title,
        html_content="<div>" + feedback + "</div>",
    )
    try:
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="feedbackHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


def trialStartHelper(user, location, code):
    message = SendGridMail(
        from_email="pipitone@fractalcomputers.com",
        to_emails=user,
        subject="Your Free Trial has Started",
        html_content=render_template("on_purchase.html", location=location, code=code),
    )
    try:
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(message)
    except Exception as e:
        fractalLog(
            function="trialStartHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    internal_message = SendGridMail(
        from_email="noreply@fractalcomputers.com",
        to_emails=["pipitone@fractalcomputers.com", "support@fractalcomputers.com"],
        subject="[FREE TRIAL START] A new user, "
        + user
        + ", just signed up for the free trial.",
        html_content="<div>No action needed from our part at this point.</div>",
    )
    try:
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(internal_message)
    except Exception as e:
        fractalLog(
            function="trialStartHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS


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
        sg = SendGridAPIClient(SENDGRID_API_KEY)
        response = sg.send(internal_message)
    except Exception as e:
        fractalLog(
            function="computerReadyHelper",
            label=user,
            logs="Mail send failed: Error code " + e.message,
            level=logging.ERROR,
        )
        return jsonify({"status": UNAUTHORIZED}), UNAUTHORIZED

    return jsonify({"status": SUCCESS}), SUCCESS

