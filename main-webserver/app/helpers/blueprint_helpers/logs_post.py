from app import *


def bookmarkHelper(connection_id):
    output = fractalSQLInsert(
        table_name="bookmarked_logs", params={"connection_id": connection_id}
    )

    return {"status": SUCCESS} if output["success"] else {"status": BAD_REQUEST}


def unbookmarkHelper(connection_id):
    output = fractalSQLDelete(
        table_name="bookmarked_logs", params={"connection_id": connection_id}
    )

    return {"status": SUCCESS} if output["success"] else {"status": BAD_REQUEST}
