from app.utils import *
from app import *
from app.logger import *


def checkComputer(computer_id, username):
    """TODO: Functions for peer to peer

    Args:
        computer_id ([type]): [description]
        username ([type]): [description]

    Returns:
        [type]: [description]
    """
    command = text(
        """
        SELECT * FROM studios WHERE "id" = :id AND "username" = :username
        """
    )
    params = {"id": computer_id, "username": username}
    with engine.connect() as conn:
        computer = cleanFetchedSQL(conn.execute(command, **params).fetchone())
        if not computer:
            computers = fetchComputers(username)
            nicknames = [c["nickname"] for c in computers]
            num = 1
            proposed_nickname = "Computer No. " + str(num)
            while proposed_nickname in nicknames:
                num += 1
                proposed_nickname = "Computer No. " + str(num)

            return {
                "userName": None,
                "location": None,
                "nickname": proposed_nickname,
                "id": None,
                "found": False,
            }

        out = {
            "username": computer[0],
            "location": computer[1],
            "nickname": computer[2],
            "id": computer[3],
            "found": True,
        }
        conn.close()
        return out


def insertComputer(username, location, nickname, computer_id):
    """TODO: Functions for peer to peer

    Args:
        username ([type]): [description]
        location ([type]): [description]
        nickname ([type]): [description]
        computer_id ([type]): [description]
    """
    computer = checkComputer(computer_id, username)
    if not computer["found"]:
        command = text(
            """
            INSERT INTO studios("username", "location", "nickname", "id")
            VALUES(:username, :location, :nickname, :id)
            """
        )

        params = {
            "username": username,
            "location": location,
            "nickname": nickname,
            "id": computer_id,
        }

        with engine.connect() as conn:
            conn.execute(command, **params)
            conn.close()


def fetchComputers(username):
    """TODO: Function for peer to peer

    Args:
        username ([type]): [description]

    Returns:
        [type]: [description]
    """
    command = text(
        """
        SELECT * FROM studios WHERE "username" = :username
        """
    )
    params = {"username": username}
    with engine.connect() as conn:
        computers = cleanFetchedSQL(conn.execute(command, **params).fetchall())
        out = [
            {
                "username": computer[0],
                "location": computer[1],
                "nickname": computer[2],
                "id": computer[3],
            }
            for computer in computers
        ]
        conn.close()
        return out
    return None


def changeComputerName(username, computer_id, nickname):
    """TODO: Function for peer to peer

    Args:
        username ([type]): [description]
        computer_id ([type]): [description]
        nickname ([type]): [description]
    """
    command = text(
        """
        UPDATE studios
        SET nickname = :nickname
        WHERE
        "username" = :username
        AND
            "id" = :id
        """
    )
    params = {"nickname": nickname, "username": username, "id": computer_id}
    with engine.connect() as conn:
        conn.execute(command, **params)
        conn.close()
