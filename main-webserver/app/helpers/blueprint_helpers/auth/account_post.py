from typing import Dict, Union, Optional

import stripe

from flask import current_app

from app.constants.http_codes import BAD_REQUEST, SUCCESS, UNAUTHORIZED
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


def login_helper(email: str, password: str) -> Dict[str, Union[bool, Optional[str], Optional[int]]]:
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
        "created_timestamp": user.created_at.timestamp(),
    }


def register_helper(
    username: str, password: str, encrypted_config_token: str, name: str, reason_for_signup: str
) -> Dict[str, Union[bool, Optional[str], Optional[int]]]:
    """Create a new Fractal user.

    Registering a new user automatically enrolls the user in a free trial of Fractal's monthly
    subscription service.

    Args:
        username: The new user's email address as a string.
        password: The digest of the user's password as a string. The user's password is hashed
            client-side. We hash this value again before storing it in the database.
        encrypted_config_token: A random string, AES-256 encrypted with the user's plaintext
            password. Encryption occurs client-side.
        name: The user's full name as a string.
        reason_for_signup: A bit of text indiciating why the user signed up for Fractal.

    Returns:
        A JSON object containing five keys: "access_token", "refresh_token", "status",
        "verification_token", and "created_timestamp". "access_token" and "refresh_token" contain
        the user's Fractal API access and refresh tokens respectively. "status" contains an integer
        that is the same as the HTTP response's status code. "verification_token" contains the
        token that the user can use to verify their email address. "created_timestamp" contains the
        UNIX timestamp representing the time at which the new user's account was created.
    """

    customer = stripe.Customer.create(email=username, name=name)
    subscription = stripe.Subscription.create(
        customer=customer["id"],
        items=[{"price": current_app.config["STRIPE_PRICE_ID"]}],
        trial_period_days=7,
    )

    assert subscription["status"] == "trialing"

    token = generate_token(username)  # Generate an email verification token.
    pwd_token = hash_value(password)

    # Add the user to the database
    new_user = User(
        user_id=username,
        encrypted_config_token=encrypted_config_token,
        password=pwd_token,
        token=token,
        name=name,
        reason_for_signup=reason_for_signup,
        stripe_customer_id=customer["id"],
    )

    access_token, refresh_token = get_access_tokens(username)

    db.session.add(new_user)
    db.session.commit()

    return {
        "status": SUCCESS,
        "verification_token": new_user.token,  # TODO: FCTL-64
        "access_token": access_token,
        "refresh_token": refresh_token,
        "created_timestamp": new_user.created_at.timestamp(),
    }


def verify_helper(username: str, provided_user_id: str) -> Dict[str, Union[bool, int]]:
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


def delete_helper(username: str) -> Dict[str, Union[int, Optional[str]]]:
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


def reset_password_helper(
    username: str, password: str, encrypted_config_token: str
) -> Dict[str, int]:
    """Updates the password for a user in the users SQL table

    Args:
        username (str): The user to update the password for
        password (str): The new password
        encrypted_config_token(str):  the new encrypted config token to store
    """
    user = User.query.get(username)

    if user:
        pwd_token = hash_value(password)
        user.password = pwd_token
        user.encrypted_config_token = encrypted_config_token

        db.session.commit()
        return {"status": SUCCESS}
    else:
        return {"status": BAD_REQUEST}


def lookup_helper(username: str) -> Dict[str, Union[bool, int]]:
    """Checks if user exists in the users SQL table

    Args:
        username (str): The user to lookup
    """
    user = User.query.get(username)

    if user:
        return {"exists": True, "status": SUCCESS}
    else:
        return {"exists": False, "status": SUCCESS}


def auto_login_helper(email: str) -> Dict[str, Union[Optional[int], Optional[str]]]:
    """
    Allows client app to pull in access and refresh tokens for staying logged in
    Args:
        email: the email to get tokens for

    Returns:  tokens to use

    """
    user = User.query.get(email)
    access_token, refresh_token = get_access_tokens(email)

    if user:
        return {
            "status": SUCCESS,
            "access_token": access_token,
            "refresh_token": refresh_token,
            "verification_token": user.token,
            "encrypted_config_token": user.encrypted_config_token,
            "name": user.name,
        }
    else:
        return {
            "status": UNAUTHORIZED,
            "access_token": None,
            "refresh_token": None,
            "verification_token": None,
            "encrypted_config_token": "",
            "name": None,
        }


def verify_password_helper(email: str, password: str) -> Dict[str, int]:
    """
    Verifies that passed in password is correct
    Args:
        email: the email to use
        password: the password to check

    Returns: a jsonified bool

    """
    user = User.query.get(email)

    if not user or not check_value(user.password, password):
        return {
            "status": UNAUTHORIZED,
        }
    else:
        return {
            "status": SUCCESS,
        }
