from app import *


def logsHelper(connection_id, username):
    session = Session()

    if connection_id:
        command = text(
            """
            SELECT * FROM logs WHERE "connection_id" = :connection_id ORDER BY last_updated DESC
            """
        )
        params = {"connection_id": connection_id}

        output = fractalCleanSQLOutput(session.execute(command, params).fetchall())
        session.commit()
        session.close()

        return {"logs": output, "status": SUCCESS}
    elif username:
        command = text(
            """
            SELECT * FROM logs WHERE "username" LIKE :username ORDER BY last_updated DESC
            """
        )
        params = {"username": username + "%"}

        output = fractalCleanSQLOutput(session.execute(command, params).fetchall())
        session.commit()
        session.close()

        return {"logs": output, "status": SUCCESS}
    else:
        command = text(
            """
            SELECT * FROM logs ORDER BY last_updated DESC
            """
        )

        params = {}

        output = fractalCleanSQLOutput(session.execute(command, params).fetchall())
        session.commit()
        session.close()

        return {"logs": output, "status": SUCCESS}
