from app.utils import *
from app import *
from app.logger import *


def loginUser(username, password):
    """Verifies the username password combination in the users SQL table

    If the password is the admin password, just check if the username exists
    Else, check to see if the username is in the database and the jwt encoded password is in the database

    Parameters:
    username (str): The username
    vm_name (str): The password

    Returns:
    bool: True if authentication success, False otherwise
   """

    if password != os.getenv("ADMIN_PASSWORD"):
        command = text(
            """
            SELECT * FROM users WHERE "username" = :userName AND "password" = :password
            """
        )
        pwd_token = jwt.encode({"pwd": password}, os.getenv("SECRET_KEY"))
        params = {"userName": username, "password": pwd_token}
        with engine.connect() as conn:
            user = cleanFetchedSQL(conn.execute(command, **params).fetchall())
            conn.close()
            return True if user else False
    else:
        command = text(
            """
            SELECT * FROM users WHERE "username" = :userName
            """
        )
        params = {"userName": username}
        with engine.connect() as conn:
            user = cleanFetchedSQL(conn.execute(command, **params).fetchall())
            conn.close()
            return True if user else False


def isAdmin(username):
    """Checks whether a user account is an admin

    Args:
        username (str): The username to check

    Returns:
        bool: True if they are an admin, False otherwise
    """
    command = text(
        """
        SELECT * FROM users WHERE "username" = :userName
        """
    )
    params = {"userName": username}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchall())[0]
        conn.close()

        if user:
            if user["password"] == jwt.encode(
                {"pwd": os.getenv("ADMIN_PASSWORD")}, os.getenv("SECRET_KEY")
            ):
                return True
        return False


def isGoogle(username):
    """Checks whether a user account used Google signup

    Args:
        username (str): The username to check

    Returns:
        bool: True if they used Google to signup, False otherwise
    """
    command = text(
        """
        SELECT * FROM users WHERE "username" = :userName
        """
    )
    params = {"userName": username}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchall())[0]
        conn.close()

        if user:
            if user["google_login"] == True:
                return True
        return False


def lookup(username):
    """Looks up the username in the users SQL table

    Args:
        username (str): The username to look up

    Returns:
        bool: True if user exists, False otherwise
    """

    command = text(
        """
        SELECT * FROM users WHERE "username" = :userName
        """
    )
    params = {"userName": username}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        return True if user else False


def genUniqueCode():
    """Generates a unique referral code

    Returns:
        int: The generated code
    """
    with engine.connect() as conn:
        old_codes = [cell[0] for cell in list(conn.execute('SELECT "code" FROM users'))]
        new_code = generateCode()
        while new_code in old_codes:
            new_code = generateCode()
        return new_code


def registerUser(username, password, token, name=None, reason_for_signup=None):
    """Registers a user, and stores it in the users table

    Args:
        username (str): The username
        password (str): The password (to be encoded into a jwt token)
        token (str): The email comfirmation token

    Returns:
        int: 200 on success, 400 on fail
    """
    pwd_token = jwt.encode({"pwd": password}, os.getenv("SECRET_KEY"))
    code = genUniqueCode()
    command = text(
        """
        INSERT INTO users("username", "password", "code", "id", "name", "reason_for_signup", "google_login")
        VALUES(:username, :password, :code, :token, :name, :reason_for_signup, :google_login)
        """
    )
    params = {
        "username": username,
        "password": pwd_token,
        "code": code,
        "token": token,
        "name": name,
        "reason_for_signup": reason_for_signup,
        "google_login": False,
        "created": dt.now(datetime.timezone.utc).timestamp(),
    }
    with engine.connect() as conn:
        try:
            conn.execute(command, **params)
            conn.close()
            return 200
        except Exception as e:
            print("Error registering user: {error}".format(error=str(e)))
            return 400


def registerGoogleUser(username, name, token, reason_for_signup=None):
    """Registers a user, and stores it in the users table

    Args:
        username (str): The username
        name (str): The user's name (from Google)
        token (str): The generated token

    Returns:
        int: 200 on success, 400 on fail
    """
    code = genUniqueCode()
    command = text(
        """
        INSERT INTO users("username", "password", "code", "id", "name", "reason_for_signup", "google_login", "verified")
        VALUES(:userName, :password, :code, :token, :name, :reason_for_signup, :google_login, :verified)
        """
    )
    params = {
        "userName": username,
        "password": None,
        "code": code,
        "token": token,
        "name": name,
        "reason_for_signup": reason_for_signup,
        "google_login": True,
        "verified": True,
    }
    with engine.connect() as conn:
        try:
            conn.execute(command, **params)
            conn.close()
            return 200
        except:
            return 400


def resetPassword(username, password):
    """Updates the password for a user in the users SQL table

    Args:
        username (str): The user to update the password for
        password (str): The new password
    """
    pwd_token = jwt.encode({"pwd": password}, os.getenv("SECRET_KEY"))
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


def deleteUser(username):
    """Deletes a user from the users sql table

    Args:
        username (str): The name of the user

    Returns:
        int: 200 for successs, 404 for failure
    """
    command = text(
        """
        DELETE FROM users WHERE "username" = :username
        """
    )
    params = {"username": username}
    with engine.connect() as conn:
        try:
            conn.execute(command, **params)
            conn.close()
            return 200
        except:
            return 404


def fetchAllUsers():
    """Fetches all users from the users sql table

    Returns:
        arr[dict]: The array of users
    """
    command = text(
        """
        SELECT * FROM users
        """
    )
    params = {}
    with engine.connect() as conn:
        users = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        return users
    return None


def changeUserCredits(username, credits):
    """Changes the outstanding credits for a user

    Args:
        username (str): The username of the user
        credits (int): The credits that the user has outstanding (1 credit = 1 month of use)
    """
    command = text(
        """
        UPDATE users
        SET "credits_outstanding" = :credits
        WHERE
        "username" = :username
        """
    )
    params = {"credits": credits, "username": username}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def getUserCredits(username):
    """Gets the credits associated with the user

    Args:
        username (str): The user's username

    Returns:
        int: The credits the user has
    """
    command = text(
        """
        SELECT * FROM users
        WHERE "username" = :username
        """
    )
    params = {"username": username}
    with engine.connect() as conn:
        users = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if users:
            return users["credits_outstanding"]
    return 0


def fetchCodes():
    """Gets all the referral codes

    Returns:
        arr[str]: An array of all the user codes
    """
    command = text(
        """
        SELECT * FROM users
        """
    )
    params = {}
    with engine.connect() as conn:
        users = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        return [user["code"] for user in users]
    return None


def checkUserVerified(username):
    """Checks if a user has verified their email already

    Args:
        username (str): The username

    Returns:
        bool: Whether they have verified
    """
    command = text(
        """
        SELECT * FROM users WHERE "username" = :userName
        """
    )
    params = {"userName": username}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if user:
            return user["verified"]
        return False


def makeUserVerified(username, verified):
    """Sets the user's verification

    Args:
        username (str): The username of the user
        verified (bool): The new verification state
    """
    command = text(
        """
        UPDATE users
        SET verified = :verified
        WHERE
        "username" = :username
        """
    )
    params = {"verified": verified, "username": username}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def fetchUserToken(username):
    """Returns the uid of the user

    Args:
        username (str): The username of the user

    Returns:
        str: The uid of the user
    """
    command = text(
        """
        SELECT * FROM users WHERE "username" = :userName
        """
    )
    params = {"userName": username}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if user:
            return user["id"]
        return None


def mapCodeToUser(code):
    """Returns the user with the respective referral code.

    Args:
        code (str): The user's referral code

    Returns:
        dict: The user. If there is no match, return None
    """
    command = text(
        """
        SELECT * FROM users WHERE "code" = :code
        """
    )
    params = {"code": code}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        return user if user else None


def fetchUserCode(username):
    """Fetches the referral code of a user

    Args:
        username (str): The username of the user of interest

    Returns:
        str: The referral code
    """
    try:
        command = text(
            """
            SELECT * FROM users WHERE "username" = :userName
            """
        )
        params = {"userName": username}
        with engine.connect() as conn:
            user = cleanFetchedSQL(conn.execute(command, **params).fetchone())
            conn.close()
            return user["code"]
    except:
        return None


def generateIDs(ID=-1):
    """Generates a unique id for all users in the users SQL table

    Returns:
        int: 200 for success, 500 for fail
    """
    try:
        with engine.connect() as conn:
            for row in list(conn.execute("SELECT * FROM users")):
                token = generateToken(row[0])
                command = text(
                    """
                    UPDATE users
                    SET "id" = :token
                    WHERE "username" = :userName
                    """
                )
                params = {"token": token, "userName": row[0]}
                conn.execute(command, **params)
            conn.close()
        sendInfo(ID, "Generated IDs succesfully")
        return 200
    except:
        error = traceback.format_exc()
        sendError(ID, "Failed to generate IDs: " + error)
        return 500


def userVMStatus(username):
    """Returns the status of the user vm

    Args:
        username (string): The username of the user of interest

    Returns:
        str: vm status ['not_created', 'is_creating', 'has_created', 'has_not_paid']
    """
    has_paid = False
    has_disk = False

    command = text(
        """
        SELECT * FROM customers
        WHERE "username" = :username
        """
    )
    params = {"username": username}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if user:
            has_paid = True

    command = text(
        """
        SELECT * FROM disks
        WHERE "username" = :username AND "state" = :state AND "main" = :main
        """
    )

    params = {"username": username, "state": "ACTIVE", "main": True}
    with engine.connect() as conn:
        user = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        if user:
            has_disk = True

            if user["first_time"]:
                print("USER {} FIRST TIME DETECTED".format(username))
                return "is_creating"

    if not has_paid and not has_disk:
        return "not_created"

    if has_paid and not has_disk:
        print("CUSTOMER {} EXISTS BUT NO DISK".format(username))
        return "is_creating"

    if has_paid and has_disk:
        return "has_created"

    return "has_not_paid"


def setUserReason(username, reason_for_signup=None):
    """Updates the reason for signup for a user in the users SQL table

    Args:
        username (str): The user to update the password for
        reason_for_signup (str): The reason the user signed up
    """
    command = text(
        """
        UPDATE users
        SET "reason_for_signup" = :reason_for_signup
        WHERE "username" = :userName
        """
    )
    params = {"userName": username, "reason_for_signup": reason_for_signup}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()
