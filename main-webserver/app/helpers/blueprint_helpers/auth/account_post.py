import datetime

from datetime import datetime as dt
from flask import current_app, jsonify

from app.constants.http_codes import BAD_REQUEST, NOT_ACCEPTABLE, SUCCESS, UNAUTHORIZED, NOT_FOUND
from app.helpers.utils.general.crypto import check_value, hash_value
from app.helpers.utils.general.logs import fractal_logger
from app.helpers.utils.general.sql_commands import (
    fractal_sql_commit,
    fractal_sql_update,
)
from app.helpers.utils.general.tokens import (
    generate_token,
    get_access_tokens,
)
from app.models import db, User
from app.helpers.utils.event_logging.events import (
    logged_event_for_logon,
)
from app.helpers.utils.mail.mail_client import MailClient


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
                "encrypted_config_token": "",
                "access_token": None,
                "refresh_token": None,
                "verification_token": None,
                "name": None,
                "created_timestamp": None,
            }

    # Fetch the JWT tokens

    access_token, refresh_token = get_access_tokens(email)

    if not current_app.testing:
        logged_event_for_logon(email)

    return {
        "verified": user.verified,
        "is_user": is_user,
        "access_token": access_token,
        "encrypted_config_token": user.encrypted_config_token,
        "refresh_token": refresh_token,
        "verification_token": user.token,
        "name": user.name,
        "created_timestamp": user.created_timestamp,
    }


def register_helper(username, password, encrypted_token, name, reason_for_signup):
    """Stores username and password in the database and generates user metadata, like their
    user ID and promo code

    Parameters:
    username (str): The username
    password (str): The password
    encrypted_token (str):  the encrypted config access token
    name (str): The person's name
    reason_for_signup (str): The person's reason for signing up

    Returns:
        A JSON object containing five keys: "access_token", "refresh_token", "status",
        "verification_token", and "created_timestamp". "access_token" and "refresh_token" contain
        the user's Fractal API access and refresh tokens respectively. "status" contains an integer
        that is the same as the HTTP response's status code. "verification_token" contains the
        token that the user can use to verify their email address. "created_timestamp" contains the
        UNIX timestamp representing the time at which the new user's account was created.
    """

    # First, generate a user ID

    token = generate_token(username)

    # Second, hash their password

    pwd_token = hash_value(password)

    # Add the user to the database
    created_timestamp = round(dt.now(datetime.timezone.utc).timestamp())

    new_user = User(
        user_id=username,
        encrypted_config_token=encrypted_token,
        password=pwd_token,
        token=token,
        name=name,
        reason_for_signup=reason_for_signup,
        created_timestamp=created_timestamp,
    )

    status = SUCCESS
    access_token, refresh_token = get_access_tokens(username)

    # Check for errors in adding the user to the database

    try:
        db.session.add(new_user)
        db.session.commit()
    except Exception:
        fractal_logger.error("Registration failed.", extra={"label": username}, exc_info=True)
        status = BAD_REQUEST
        access_token = refresh_token = None

    return {
        "status": status,
        "verification_token": new_user.token if status == SUCCESS else None,  # TODO: Issue 519
        "access_token": access_token,
        "refresh_token": refresh_token,
        "created_timestamp": created_timestamp if status == SUCCESS else None,
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
            fractal_logger.info("Verification token is valid, verifying.", extra={"label": user_id})
            fractal_sql_commit(db, fractal_sql_update, user, {"verified": True})

            return {"status": SUCCESS, "verified": True}
        else:
            fractal_logger.warning(
                "Verification token {token} is invalid, cannot validate.".format(
                    token=provided_user_id
                ),
                extra={"label": user_id},
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
        old_config_token = user.encrypted_config_token
        pwd_token = hash_value(password)

        user.password = pwd_token
        db.session.commit()
        return {"status": SUCCESS, "old_token": old_config_token}
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
            url = current_app.config["FRONTEND_URL"] + "/verify?" + token
            mail_client = MailClient(current_app.config["SENDGRID_API_KEY"])
            mail_client.send_email(
                to_email=user.user_id, email_id="EMAIL_VERIFICATION", jinja_args={"url": url}
            )
        if "password" in body:
            result_dict = reset_password_helper(body["username"], body["password"])
            if result_dict["status"] == SUCCESS:
                result_dict["msg"] = "Password updated successfully"
            else:
                result_dict["msg"] = "We had an error updating your password"
            return jsonify(result_dict), SUCCESS
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
