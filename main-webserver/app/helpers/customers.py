from app import *
from app.utils import *
from app.logger import *


def fetchCustomer(username):
    """Fetches the customer from the customers sql table by username

    Args:
        username (str): The customer name

    Returns:
        dict: A dictionary that represents the customer
    """
    command = text(
        """
        SELECT * FROM customers WHERE "username" = :username
        """
    )
    params = {"username": username}
    with engine.connect() as conn:
        customer = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        return customer


def fetchCustomerById(id):
    """Fetches the customer from the customers sql table by id

    Args:
        id (str): The unique id of the customer

    Returns:
        dict: A dictionary that represents the customer
    """
    command = text(
        """
        SELECT * FROM customers WHERE "id" = :id
        """
    )
    params = {"id": id}
    with engine.connect() as conn:
        customer = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        conn.close()
        return customer


def fetchCustomers():
    """Fetches all customers from the customers sql table

    Returns:
        arr[dict]: An array of all the customers
    """
    command = text(
        """
        SELECT * FROM customers
        """
    )
    params = {}
    with engine.connect() as conn:
        customers = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        conn.close()
        return customers


def insertCustomer(
    email, customer_id, subscription_id, location, trial_end, paid, ID=-1
):
    """Adds a customer to the customer sql table, or updates the row if the customer already exists

    Args:
        email (str): Email of the customer
        customer_id (str): Uid of the customer
        subscription_id (str): Uid of the customer's subscription, if applicable
        location (str): The location of the customer (state)
        trial_end (int): The unix timestamp of the expiry of their trial
        paid (bool): Whether or not the user paid before
    """
    sendInfo(
        ID,
        "Adding customer to sql table with email {}, customer_id {}, subscription_id {}, location {}, trial_end {}, paid{}".format(
            email, customer_id, subscription_id, location, trial_end, paid
        ),
    )
    command = text(
        """
        SELECT * FROM customers WHERE "username" = :email
        """
    )
    params = {"email": email}
    with engine.connect() as conn:
        customers = cleanFetchedSQL(conn.execute(command, **params).fetchall())

        if not customers:
            command = text(
                """
                INSERT INTO customers("username", "id", "subscription", "location", "trial_end", "paid")
                VALUES(:email, :id, :subscription, :location, :trial_end, :paid)
                """
            )

            params = {
                "email": email,
                "id": customer_id,
                "subscription": subscription_id,
                "location": location,
                "trial_end": trial_end,
                "paid": paid,
            }

            conn.execute(command, **params)
            conn.close()
        else:
            location = customers[0]["location"]
            command = text(
                """
                UPDATE customers
                SET "id" = :id,
                    "subscription" = :subscription,
                    "location" = :location,
                    "trial_end" = :trial_end,
                    "paid" = :paid
                WHERE "username" = :email
                """
            )

            params = {
                "email": email,
                "id": customer_id,
                "subscription": subscription_id,
                "location": location,
                "trial_end": trial_end,
                "paid": paid,
            }

            conn.execute(command, **params)
            conn.close()


def deleteCustomer(username):
    """Deletes a cusotmer from the table

    Args:
        username (str): The username (email) of the customer
    """
    command = text(
        """
        DELETE FROM customers WHERE "username" = :email
        """
    )
    params = {"email": username}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def updateTrialEnd(subscription, trial_end):
    """Update the end date for the trial subscription

    Args:
        subscription (str): The uid of the subscription we wish to update
        trial_end (int): The trial end date, as a unix timestamp
    """
    command = text(
        """
        UPDATE customers
        SET trial_end = :trial_end
        WHERE
        "subscription" = :subscription
        """
    )
    params = {"subscription": subscription, "trial_end": trial_end}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()


def addPendingCharge(username, amount, ID=0):
    """Adds to the user's current pending charges by amount. This is done since stripe requires payments of at least 50 cents. By using pending charges, we can keep track and charge it all at the end.

    Args:
        username (str): The username of the user to add a pending charge to
        amount (int): Amount by which to increment by
        ID (int, optional): Papertrail logging ID. Defaults to 0.
    """
    command = text(
        """
        SELECT *
        FROM customers
        WHERE "username" = :username
        """
    )

    params = {"username": username}

    with engine.connect() as conn:
        customer = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        if customer:
            pending_charges = customer["pending_charges"] + amount
            command = text(
                """
                UPDATE customers
                SET "pending_charges" = :pending_charges
                WHERE "username" = :username
                """
            )
            params = {"pending_charges": pending_charges, "username": username}

            conn.execute(command, **params)
            conn.close()
        else:
            sendCritical(
                ID,
                "{} has an hourly plan but was not found as a customer in database".format(
                    username
                ),
            )

