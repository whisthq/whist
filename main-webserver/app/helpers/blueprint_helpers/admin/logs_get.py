from app import *


def logsHelper(connection_id, username, bookmarked):
    session = Session()

    if bookmarked:
        output = fractalSQLSelect(table_name="protocol_logs", params={"bookmarked": True })

        connection_ids = []
        if output:
            connection_ids = [element["connection_id"] for element in output]

        return {"logs": output, "connection_ids": connection_ids, "status": SUCCESS}
    if connection_id:
        rows = (
            session.query(ProtocolLog).filter(ProtocolLog.connection_id == connection_id).order_by(ProtocolLog.timestamp)
        )


        session.commit()
        session.close()

        rows = rows.all()
        output = []
        for row in rows:
            row.__dict__.pop("_sa_instance_state", None)
            output.append(row.__dict__)

        return {"logs": output, "status": SUCCESS}
    elif username:
        users = fractalSQLSelect(table_name="users", params={"email": username})

        user_id = users["rows"][0]["user_id"]

        rows = (
            session.query(ProtocolLog).filter(ProtProtocolLog.user_id == user_id).order_by(ProtocolLog.timestamp)
        )


        session.commit()
        session.close()

        rows = rows.all()
        output = []
        for row in rows:
            row.__dict__.pop("_sa_instance_state", None)
            output.append(row.__dict__)


        return {"logs": output, "status": SUCCESS}
    else:
        output = session.query(ProtocolLog).order_by(ProtocolLog.timestamp)

        session.commit()
        session.close()

        return {"logs": output, "status": SUCCESS}
