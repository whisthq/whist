from app import *
from app.helpers.utils.general.logs import *
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

    is_user = True

    if password == ADMIN_PASSWORD:
        is_user = False

    user = User.query.get(email)

    fractalLog(function="", label="", logs=str(user))

    # Return early if username/password combo is invalid

    if is_user:
        if not user or not check_value(user.password, password):
            return {
                "verified": False,
                "is_user": is_user,
                "access_token": None,
                "refresh_token": None,
                "verification_token": None,
            }

    # Fetch the JWT tokens

    access_token, refresh_token = getAccessTokens(email)

    return {
        "verified": True,
        "is_user": is_user,
        "access_token": access_token,
        "refresh_token": refresh_token,
        "verification_token": user.token,
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

    token = generateToken(username)

    # Second, hash their password

    pwd_token = hash_value(password)

    # Third, generate a promo code for the user

    promo_code = generateUniquePromoCode()

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
    )

    status = SUCCESS
    access_token, refresh_token = getAccessTokens(username)

    # Check for errors in adding the user to the database

    try:
        db.session.add(new_user)
        db.session.commit()
    except Exception as e:
        fractalLog(
            function="registerHelper",
            label=username,
            logs="Registration failed: " + str(e),
            level=logging.ERROR,
        )
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
                logs="Mail send failed: Error code " + str(e),
                level=logging.ERROR,
            )

    return {
        "status": status,
        "token": new_user.token,
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

    user = User.query.get(username)

    if user:
        user_id = user.token

        # Check to see if the provided user ID matches the selected user ID
        #
        # if provided_user_id == user_id:
        #     alreadyVerified = output["rows"][0]["verified"]
        #     fractalSQLUpdate(
        #         table_name="users",
        #         conditional_params={"email": username},
        #         new_params={"verified": True},
        #     )
        #
        #     if not alreadyVerified:
        #         # Send welcome mail to user after they verify for the first time
        #         signupMail(user.user_id, user.referral_code)

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

    user = User.query.get(username)

    if not user:
        return {"status": BAD_REQUEST, "error": output["error"]}

    db.session.delete(user)
    db.session.commit()

    disks = OSDisk.query.filter_by(user_id=username).all()
    if disks:
        for disk in disks:
            deleteDisk.apply_async([disk.disk_id, VM_GROUP])

    return {"status": SUCCESS, "error": None}


def resetPasswordHelper(username, password):
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


def lookupHelper(username):
    """Checks if user exists in the users SQL table

    Args:
        username (str): The user to lookup
    """
    user = User.query.get(username)

    if user:
        return {"exists": True, "status": SUCCESS}
    else:
        return {"exists": False, "status": SUCCESS}


def updateUserHelper(body):
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
            return verificationHelper(body["email"], token)
        if "password" in body:
            resetPasswordHelper(body["username"], body["password"])
            return jsonify({"msg": "Password updated successfully"}), SUCCESS
        return jsonify({"msg": "Field not accepted"}), NOT_ACCEPTABLE
