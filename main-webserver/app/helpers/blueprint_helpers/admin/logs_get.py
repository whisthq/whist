from app import *

from app.models.logs import *
from app.models.public import *

from app.serializers.logs import *

def logsHelper(connection_id, username, bookmarked):
    log_schema = ProtocolLogSchema()

    if bookmarked:
        logs = ProtocolLog.query.filter_by(bookmarked=True).all()

        connection_ids = []
        if logs:
            connection_ids = [log.connection_id for log in logs]

        logs = [log_schema.dump(log) for log in logs]

        return {"logs": logs, "connection_ids": connection_ids, "status": SUCCESS}
    if connection_id:
        logs = ProtocolLog.filter_by(connection_id=connection_id).order_by(ProtocolLog.timestamp).all()
        logs = [log_schema.dump(log) for log in logs]

        return {"logs": logs, "status": SUCCESS}
    elif username:
        user = User.query.filter_by(email=username).first()
        user_id = user.user_id

        logs = ProtocolLog.query.filer_by(user_id=user_id).order_by(ProtocolLog.timestamp).all()
        logs = [log_schema.dump(log) for log in logs]

        return {"logs": logs, "status": SUCCESS}
    else:
        logs = ProtocolLog.query.order_by(ProtocolLog.timestamp).all()
        logs = [log_schema.dump(log) for log in logs]

        return {"logs": logs, "status": SUCCESS}
