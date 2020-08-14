from app import *
from app.helpers.utils.general.logs import *
from app.helpers.utils.general.sql_commands import *
from app.helpers.utils.general.tokens import *
from app.helpers.utils.mail.account_mail import *
from app.helpers.utils.general.crypto import *
from app.helpers.blueprint_helpers.mail.mail_post import *
from app.celery.azure_resource_deletion import *


def loginHelper(email, password):
    """Verifies the username password combination in the users SQL table

    If the password is the admin password, just check if the username exists
    Else, check to see if the username is in the database and the jwt encoded password is in the database

    Parameters:
    email (str): The email
    password (str): The password

    Returns:
    json: Metadata about the user
   """

    # First, check if username is valid

    params = {"email": email}

    is_user = True

    if password == ADMIN_PASSWORD:
        is_user = False

    output = fractalSQLSelect("users", params)

    # Return early if username/password combo is invalid

    if is_user:
        if not output or not check_value(output[0]["password_token"], password):
            return {
                "verified": False,
                "is_user": is_user,
                "access_token": None,
                "refresh_token": None,
            }

    # Fetch the JWT tokens

    access_token, refresh_token = getAccessTokens(email)

    return {
        "verified": True,
        "is_user": is_user,
        "access_token": access_token,
        "refresh_token": refresh_token,
    }


def registerHelper(username, password, name, reason_for_signup):
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

    user_id = generateToken(username)

    # Second, hash their password

    pwd_token = hash_value(password)

    # Third, generate a promo code for the user

    promo_code = generateUniquePromoCode()

    # Add the user to the database

    params = {
        "email": username,
        "password_token": pwd_token,
        "code": promo_code,
        "id": user_id,
        "name": name,
        "reason_for_signup": reason_for_signup,
        "created": dt.now(datetime.timezone.utc).timestamp(),
    }

    unique_keys = {"email": username}

    output = fractalSQLInsert("users", params, unique_keys=unique_keys)
    status = SUCCESS
    access_token, refresh_token = getAccessTokens(username)

    # Check for errors in adding the user to the database

    if not output["success"] and "already exists" in output["error"]:
        status = CONFLICT
        user_id = access_token = refresh_token = None
    elif not output["success"]:
        status = BAD_REQUEST
        user_id = access_token = refresh_token = None

    if status == SUCCESS:
        internal_message = SendGridMail(
            from_email="support@fractalcomputers.com",
            to_emails="support@fractalcomputers.com",
            subject=username + " just created an account!",
            html_content="<p>Just letting you know that {0} created an account. Their reason for signup is: {1}. Have a great day.</p>".format(
                name, reason_for_signup
            ),
        )

        try:
            sg = SendGridAPIClient(SENDGRID_API_KEY)
            response = sg.send(internal_message)
        except Exception as e:
            fractalLog(
                function="registerHelper",
                label=username,
                logs="Mail send failed: Error code " + e.message,
                level=logging.ERROR,
            )

    return {
        "status": status,
        "token": user_id,
        "access_token": access_token,
        "refresh_token": refresh_token,
    }


def verifyHelper(username, provided_user_id):
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

    output = fractalSQLSelect(table_name="users", params={"email": username})

    if output:
        user_id = output[0]["user_id"]

    # Check to see if the provided user ID matches the selected user ID

    if provided_user_id == user_id:
        alreadyVerified = output["rows"][0]["verified"]
        fractalSQLUpdate(
            table_name="users",
            conditional_params={"email": username},
            new_params={"verified": True},
        )

        if not alreadyVerified:
            # Send welcome mail to user after they verify for the first time
            signupMail(output["rows"][0]["email"], output["rows"][0]["code"])

        return {"status": SUCCESS, "verified": True}
    else:
        return {"status": UNAUTHORIZED, "verified": False}


def deleteHelper(username):
    """Deletes a user's account and their disks from the database

    Parameters:
    username (str): The username

    Returns:
    json: Success/failure of deletion
   """
    output = fractalSQLDelete("users", {"email": username})

    if not output["success"]:
        return {"status": BAD_REQUEST, "error": output["error"]}

    disks = fractalSQLSelect("disks", {"email": username})["rows"]
    if disks:
        for disk in disks:
            deleteDisk.apply_async([disk["disk_name"], VM_GROUP])

    return {"status": SUCCESS, "error": None}


def resetPasswordHelper(username, password):
    """Updates the password for a user in the users SQL table

    Args:
        username (str): The user to update the password for
        password (str): The new password
    """
    # TODO: NEEDS TO BE CHANGED TO SHA256, NOT JWT
    pwd_token = jwt.encode({"pwd": password}, SHA_SECRET_KEY)
    fractalSQLUpdate(
        table_name="users",
        conditional_params={"email": username},
        new_params={"password": pwd_token},
    )


def lookupHelper(username):
    """Checks if user exists in the users SQL table

    Args:
        username (str): The user to lookup
    """
    params = {
        "email": username,
    }

    output = fractalSQLSelect("users", params)

    if output["success"]:
        if output["rows"]:
            return {"exists": True, "status": SUCCESS}
        else:
            return {"exists": False, "status": SUCCESS}
    else:
        return {"status": BAD_REQUEST}


def updateUserHelper(body):
    if "name" in body:
        fractalSQLUpdate(
            table_name="users",
            conditional_params={"email": body["email"]},
            new_params={"name": body["name"]},
        )
        return jsonify({"msg": "Name updated successfully"}), SUCCESS
    if "email" in body:
        fractalSQLUpdate(
            table_name="users",
            conditional_params={"email": body["email"]},
            new_params={"email": body["email"], "verified": False},
        )
        token = fractalSQLSelect("users", {"email": body["email"]})["rows"][0]["id"]
        return verificationHelper(body["email"], token)
    if "password" in body:
        resetPasswordHelper(body["email"], body["password"])
        return jsonify({"msg": "Password updated successfully"}), SUCCESS
    return jsonify({"msg": "Field not accepted"}), NOT_ACCEPTABLE
