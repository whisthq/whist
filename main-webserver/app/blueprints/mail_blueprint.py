from app import *

from app.logger import *
from app.helpers.mail import *

mail_bp = Blueprint("mail_bp", __name__)


@mail_bp.route("/mail/<action>", methods=["POST"])
@logRequestInfo
def mail(action, **kwargs):
    body = request.get_json()
    if action == "forgot":
        username = body["username"]
        command = text(
            """
            SELECT * FROM users WHERE "username" = :userName
            """
        )
        params = {"userName": username}
        with engine.connect() as conn:
            user = conn.execute(command, **params).fetchall()
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
                    sendError(kwargs["ID"], "Mail send failed: Error code " + e.message)
                    return jsonify({"status": 500}), 500

                command = text(
                    """
                    INSERT INTO password_tokens("token", "time_issued") 
                    VALUES(:token, :time_issued)
                    """
                )
                params = {
                    "token": token,
                    "time_issued": dt.now().strftime("%m-%d-%Y, %H:%M:%S"),
                }
                conn.execute(command, **params)
                conn.close()
                return jsonify({"verified": verified}), 200
            else:
                conn.close()
                return jsonify({"verified": verified}), 401

    elif action == "reset":
        username, password = body["username"], body["password"]
        pwd_token = jwt.encode({"pwd": password}, SECRET_KEY)
        command = text(
            """
            UPDATE users 
            SET "password" = :password
            WHERE "username" = :userName
            """
        )
        params = {"userName": username, "password": pwd_token}
        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()
        return jsonify({"status": 200}), 200
    elif action == "cancel":
        body = request.get_json()
        user, feedback = body["username"], body["feedback"]
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
            sendError(kwargs["ID"], "Mail send failed: Error code " + e.message)

        return jsonify({"status": 200}), 200
    elif action == "verification":
        body = request.get_json()
        user, token = body["username"], body["token"]
        title = "[Fractal] Please Verify Your Email"
        url = os.getenv("FRONTEND_URL") + "/verify?" + token

        print("THE URL IS {}".format(url))

        internal_message = SendGridMail(
            from_email="noreply@fractalcomputers.com",
            to_emails=user,
            subject=title,
            html_content=render_template("on_email_verification.html", url=url),
        )

        try:
            sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
            response = sg.send(internal_message)
            sendInfo(kwargs["ID"], "Sent email to {}".format(user))
        except Exception as e:
            sendError(kwargs["ID"], "Mail send failed: Error code " + e.message)

        return jsonify({"status": 200}), 200
    elif action == "referral":
        body = request.get_json()
        user = body["username"]
        title = "Check out Fractal"

        internal_message = SendGridMail(
            from_email=user,
            to_emails=body["recipients"],
            subject=title,
            html_content=render_template(
                "on_referral.html", code=body["code"], user=user
            ),
        )

        try:
            sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
            response = sg.send(internal_message)
        except Exception as e:
            sendError(kwargs["ID"], "Mail send failed: Error code " + e.message)

        return jsonify({"status": 200}), 200


@mail_bp.route("/token/validate", methods=["POST"])
@logRequestInfo
def validateToken(**kwargs):
    body = request.get_json()
    command = text(
        """
        SELECT * FROM password_tokens WHERE "token" = :token
        """
    )
    params = {"token": body["token"]}
    with engine.connect() as conn:
        user = conn.execute(command, **params).fetchall()
        if user:
            timeIssued = user[0][1]
            diff = (
                dt.now() - dt.strptime(timeIssued, "%m-%d-%Y, %H:%M:%S")
            ).total_seconds()
            conn.close()
            if diff > (60 * 10):
                return jsonify({"status": 401, "error": "Expired token"}), 401
            else:
                return jsonify({"status": 200}), 200
        else:
            conn.close()
            return jsonify({"status": 401, "error": "Invalid token"}), 401


@mail_bp.route("/feedback", methods=["POST"])
def feedback():
    body = request.get_json()
    user = body["username"]
    feedback_type = body["type"]
    title = "[{}] Feedback from " + user.format(feedback_type)

    message = SendGridMail(
        from_email="noreply@fractalcomputers.com",
        to_emails=["pipitone@fractalcomputers.com", "support@fractalcomputers.com"],
        subject=title,
        html_content="<div>" + body["feedback"] + "</div>",
    )
    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(message)
    except Exception as e:
        print(e.message)

    return jsonify({"status": 200}), 200


@mail_bp.route("/trial/<action>", methods=["POST"])
@generateID
@logRequestInfo
def trial(action, **kwargs):
    body = request.get_json()
    if action == "start":
        user, location, code = body["username"], body["location"], body["code"]
        message = SendGridMail(
            from_email="pipitone@fractalcomputers.com",
            to_emails=user,
            subject="Your Cloud PC Is Being Prepared",
            html_content=render_template(
                "on_purchase.html", location=location, code=code
            ),
        )
        try:
            sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
            response = sg.send(message)
        except Exception as e:
            print(e.message)

        internal_message = SendGridMail(
            from_email="noreply@fractalcomputers.com",
            to_emails=["pipitone@fractalcomputers.com", "support@fractalcomputers.com"],
            subject=user + " signed up for a free trial",
            html_content="<div>"
            + user
            + " has created a trial pc in "
            + body["location"]
            + "</div>",
        )
        try:
            sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
            response = sg.send(internal_message)
        except Exception as e:
            print(e.message)

    return jsonify({"status": 200}), 200


# TODO: Check to see if this endpoint is deprecated
@mail_bp.route("/purchase", methods=["POST"])
def purchase():
    body = request.get_json()
    user, code = body["username"], body["code"]
    message = SendGridMail(
        from_email="pipitone@fractalcomputers.com",
        to_emails=user,
        subject="Your Free Trial has Started",
        html_content=render_template("on_purchase.html", location=location, code=code),
    )
    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(message)
    except Exception as e:
        print(e.message)

    title = (
        "[FREE TRIAL START] A new user, "
        + user
        + ", just signed up for the free trial."
    )

    internal_message = SendGridMail(
        from_email="noreply@fractalcomputers.com",
        to_emails=["pipitone@fractalcomputers.com", "support@fractalcomputers.com"],
        subject=title,
        html_content="<div>No action needed from our part at this point.</div>",
    )
    try:
        sg = SendGridAPIClient(os.getenv("SENDGRID_API_KEY"))
        response = sg.send(internal_message)
    except Exception as e:
        print(e.message)

    return jsonify({"status": 200}), 200


@mail_bp.route("/signup", methods=["POST"])
def signup():
    body = request.get_json()
    user, code = body["username"], body["code"]
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
        print("Email sent to {}".format(user))
        print(response)
    except Exception as e:
        print(e.message)

    return jsonify({"status": 200}), 200


@mail_bp.route("/newsletter/<action>", methods=["POST"])
def newsletter(action):
    if action == "subscribe":
        body = request.get_json()

        command = text(
            """
            INSERT INTO newsletter("username") 
            VALUES(:username)
            """
        )

        params = {"username": body["username"]}
        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()
        return jsonify({"status": 200}), 200

    elif action == "unsubscribe":
        body = request.get_json()

        command = text(
            """
            DELETE FROM newsletter WHERE "username" = :username 
            """
        )

        params = {"username": body["username"]}
        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()
        return jsonify({"status": 200}), 200


# TODO: Check to see if this endpoint is deprecated
@mail_bp.route("/computerReady", methods=["POST"])
def computerReady():
    body = request.get_json()
    user, date, code, location = (
        body["username"],
        body["date"],
        body["code"],
        body["location"],
    )
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
        print(e.message)

    return jsonify({"status": 200}), 200
