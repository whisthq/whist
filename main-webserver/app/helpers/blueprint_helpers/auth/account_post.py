import datetime
import logging

from datetime import datetime as dt
from flask import current_app, jsonify
from sendgrid import SendGridAPIClient
from sendgrid.helpers.mail import Mail

from app.constants.http_codes import BAD_REQUEST, NOT_ACCEPTABLE, SUCCESS, UNAUTHORIZED, NOT_FOUND
from app.helpers.blueprint_helpers.mail.mail_post import verification_helper
from app.helpers.utils.general.crypto import check_value, hash_value
from app.helpers.utils.general.logs import fractal_log
from app.helpers.utils.general.sql_commands import (
    fractal_sql_commit,
    fractal_sql_update,
)
from app.helpers.utils.general.tokens import (
    generate_token,
    generate_unique_promo_code,
    get_access_tokens,
)
from app.models import db, User

from app.helpers.utils.datadog.events import (
    datadogEvent_userLogon,
)


def login_helper(email, password):
    """Verifies the username password combination in the users SQL table

    If the password is the admin password, just check if the username exists
    Otherwise, check to see if the username is in the database and the JWT
    encoded password is in the database

    Parameters:
    email (str): The email
    password (str): The password

    Returns:
    json: Metadata about the user
    """

    # First, check if username is valid

    is_user = True

    user = User.query.get(email)

    # Return early if username/password combo is invalid

    if is_user:
        if not user or not check_value(user.password, password):
            return {
                "verified": False,
                "is_user": is_user,
                "access_token": None,
                "refresh_token": None,
                "verification_token": None,
                "name": None,
                "can_login": False,
                "card_brand": None,
                "card_last_four": None,
                "postal_code": None,
            }

    # Fetch the JWT tokens

    access_token, refresh_token = get_access_tokens(email)

    if not current_app.testing:
        datadogEvent_userLogon(email)

    return {
        "verified": user.verified,
        "is_user": is_user,
        "access_token": access_token,
        "refresh_token": refresh_token,
        "verification_token": user.token,
        "name": user.name,
        "can_login": user.can_login,
        "card_brand": user.card_brand,
        "card_last_four": user.card_last_four,
        "postal_code": user.postal_code,
    }


def register_helper(username, password, name, reason_for_signup, can_login):
    """Stores username and password in the database and generates user metadata, like their
    user ID and promo code

    Parameters:
    username (str): The username
    password (str): The password
    name (str): The person's name
    reason_for_signup (str): The person's reason for signing up

    Returns:
    json: Metadata about the user
    """

    # First, generate a user ID

    token = generate_token(username)

    # Second, hash their password

    pwd_token = hash_value(password)

    # Third, generate a promo code for the user

    promo_code = generate_unique_promo_code()

    # Add the user to the database

    new_user = User(
        user_id=username,
        password=pwd_token,
        token=token,
        referral_code=promo_code,
        name=name,
        reason_for_signup=reason_for_signup,
        release_stage=50,
        created_timestamp=dt.now(datetime.timezone.utc).timestamp(),
        can_login=can_login,
    )

    status = SUCCESS
    access_token, refresh_token = get_access_tokens(username)

    # Check for errors in adding the user to the database

    try:
        db.session.add(new_user)
        db.session.commit()
    except Exception as e:
        fractal_log(
            function="register_helper",
            label=username,
            logs="Registration failed: " + str(e),
            level=logging.ERROR,
        )
        status = BAD_REQUEST
        access_token = refresh_token = None

    if status == SUCCESS:
        try:
            message = Mail(
                from_email=current_app.config["SENDGRID_DEFAULT_FROM"],
                to_emails="support@tryfractal.com",
                subject=username + " just created an account!",
                html_content=(
                    f"<p>Just letting you know that {name} created an account. Their reason for "
                    f"signup is: {reason_for_signup}. Have a great day.</p>"
                ),
            )
            sendgrid_client = SendGridAPIClient(current_app.config["SENDGRID_API_KEY"])

            sendgrid_client.send(message)
        except Exception as e:
            fractal_log(
                function="register_helper",
                label=username,
                logs="Mail send failed: Error code " + str(e),
                level=logging.ERROR,
            )

    return {
        "status": status,
        "verification_token": new_user.token,
        "access_token": access_token,
        "refresh_token": refresh_token,
    }


def verify_helper(username, provided_user_id):
    """Checks provided verification token against database token. If they match, we verify the
    user's email.

    Parameters:
    username (str): The username
    token (str): Email verification token

    Returns:
    json: Success/failure of email verification
    """

    # Select the user's ID

    user_id = None

    user = User.query.filter_by(user_id=username).first()

    if user:
        # Check to see if the provided user ID matches the selected user ID

        if provided_user_id == user.token:
            fractal_log(
                function="verify_helper",
                label=user_id,
                logs="Verification token is valid, verifying.",
            )
            fractal_sql_commit(db, fractal_sql_update, user, {"verified": True})

            return {"status": SUCCESS, "verified": True}
        else:
            fractal_log(
                function="verify_helper",
                label=user_id,
                logs="Verification token {token} is invalid, cannot validate.".format(
                    token=provided_user_id
                ),
                level=logging.WARNING,
            )
            return {"status": UNAUTHORIZED, "verified": False}
    else:
        return {"status": UNAUTHORIZED, "verified": False}


def delete_helper(username):
    """Deletes a user's account and their disks from the database

    Parameters:
    username (str): The username

    Returns:
    json: Success/failure of deletion
    """

    user = User.query.get(username)

    if not user:
        return {"status": BAD_REQUEST, "error": f"User {username} not found"}

    db.session.delete(user)
    db.session.commit()

    return {"status": SUCCESS, "error": None}


def reset_password_helper(username, password):
    """Updates the password for a user in the users SQL table

    Args:
        username (str): The user to update the password for
        password (str): The new password
    """
    user = User.query.get(username)

    if user:
        pwd_token = hash_value(password)

        user.password = pwd_token
        db.session.commit()
        return {"status": SUCCESS}
    else:
        return {"status": BAD_REQUEST}


def lookup_helper(username):
    """Checks if user exists in the users SQL table

    Args:
        username (str): The user to lookup
    """
    user = User.query.get(username)

    if user:
        return {"exists": True, "status": SUCCESS}
    else:
        return {"exists": False, "status": SUCCESS}


def update_user_helper(body):
    user = User.query.get(body["username"])
    if user:
        if "name" in body:
            user.name = body["name"]
            db.session.commit()

            return jsonify({"msg": "Name updated successfully"}), SUCCESS
        if "email" in body:
            user.user_id = body["email"]
            db.session.commit()

            token = user.token
            return verification_helper(body["email"], token)
        if "password" in body:
            reset_password_helper(body["username"], body["password"])
            return jsonify({"msg": "Password updated successfully"}), SUCCESS
        return jsonify({"msg": "Field not accepted"}), NOT_ACCEPTABLE
    return jsonify({"msg": "User not found"}), NOT_FOUND


def auto_login_helper(email):
    user = User.query.get(email)
    access_token, refresh_token = get_access_tokens(email)

    if user:
        return {
            "status": SUCCESS,
            "access_token": access_token,
            "refresh_token": refresh_token,
            "verification_token": user.token,
            "name": user.name,
        }
    else:
        return {
            "status": UNAUTHORIZED,
            "access_token": None,
            "refresh_token": None,
            "verification_token": None,
            "name": None,
        }


def verify_password_helper(email, password):
    user = User.query.get(email)

    if not user or not check_value(user.password, password):
        return {
            "status": UNAUTHORIZED,
        }
    else:
        return {
            "status": SUCCESS,
        }
