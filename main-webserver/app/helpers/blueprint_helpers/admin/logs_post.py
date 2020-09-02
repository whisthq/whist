from app import *

from app.models.logs import *


def bookmarkHelper(connection_id):

    log = ProtocolLog.query.filter_by(connection_id=str(connection_id)).first()
    log.bookmarked = True
    db.session.commit()

    return {"status": SUCCESS}


def unbookmarkHelper(connection_id):
    log = ProtocolLog.query.filter_by(connection_id=str(connection_id)).first()
    log.bookmarked = False
    db.session.commit()

    return {"status": SUCCESS}
