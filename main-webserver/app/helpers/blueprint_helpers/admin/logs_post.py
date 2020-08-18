from app import *


def bookmarkHelper(connection_id):
    output = fractalSQLUpdate(
        table_name="protocol_logs", conditional_params={"connection_id": connection_id},
        new_params={"bookmarked": True}
    )

    return {"status": SUCCESS} if output["success"] else {"status": BAD_REQUEST}


def unbookmarkHelper(connection_id):
    output = fractalSQLUpdate(
        table_name="protocol_logs", conditional_params={"connection_id": connection_id},
        new_params={"bookmarked": False}
    )

    return {"status": SUCCESS} if output["success"] else {"status": BAD_REQUEST}
