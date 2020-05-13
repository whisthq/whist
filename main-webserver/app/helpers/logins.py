from app.utils import *
from app import *
from app.logger import *
from .vms import *
from .disks import *


def addTimeTable(username, action, time, is_user, ID=-1):
    """Adds a user action to the login_history sql table

    Args:
        username (str): The username of the user
        action (str): ['logon', 'logoff']
        time (str): Time in the format mm-dd-yyyy, hh:mm:ss
        is_user (bool): Whether an actual user did the action, vs admin
        ID (int, optional): Papertrail loggind ID. Defaults to -1.
    """
    command = text("""
        INSERT INTO login_history("username", "timestamp", "action", "is_user")
        VALUES(:userName, :currentTime, :action, :is_user)
        """)

    aware = dt.now()
    now = aware.strftime('%m-%d-%Y, %H:%M:%S')

    sendInfo(ID, 'Adding to time table a {} at time {}'.format(action, now))

    params = {'userName': username, 'currentTime': now,
              'action': action, 'is_user': is_user}

    with engine.connect() as conn:
        conn.execute(command, **params)

        disk = fetchUserDisks(username)
        if disk:
            disk_name = disk[0]['disk_name']
            vms = mapDiskToVM(disk_name)
            if vms:
                vm_name = vms[0]['vm_name']

                if action == 'logoff':
                    lockVM(vm_name, False)
                elif action == 'logon':
                    lockVM(vm_name, True)
            else:
                sendCritical(
                    ID, 'Could not find a VM currently attached to disk {}'.format(disk_name))
        else:
            sendCritical(
                ID, 'Could not find a disk belong to user {}'.format(username))

        conn.close()


def fetchLoginActivity():
    """Fetches the entire login_history sql table

    Returns:
        array[dict]: The login history
    """
    command = text("""
        SELECT * FROM login_history
        """)
    params = {}
    with engine.connect() as conn:
        activities = cleanFetchedSQL(
            conn.execute(command, **params).fetchall())
        activities.reverse()
        conn.close()
        return activities


def getMostRecentActivity(username):
    """Gets the last activity of a user

    Args:
        username (str): Username of the user

    Returns:
        str: The latest activity of the user
    """
    command = text("""
        SELECT *
        FROM login_history
        WHERE "username" = :username
        ORDER BY timestamp DESC LIMIT 1
        """)

    params = {'username': username}

    with engine.connect() as conn:
        activity = cleanFetchedSQL(
            conn.execute(command, **params).fetchone())
        return activity
