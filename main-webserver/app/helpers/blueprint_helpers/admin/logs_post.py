from app import *

from app.models.logs import *


def bookmarkHelper(connection_id):
    try:
        log = ProtocolLog.query.filter_by(connection_id=connection_id).first()
        log.bookmarked = True
        db.session.commit()

        return {"status": SUCCESS}
    except Exception:
        return {"status": BAD_REQUEST}


def unbookmarkHelper(connection_id):
        try:
            log = ProtocolLog.query.filter_by(connection_id=connection_id).first()
            log.bookmarked = False
            db.session.commit()

            return {"status": SUCCESS}
        except Exception:
            return {"status": BAD_REQUEST}
