from app.constants.http_codes import SUCCESS
from app.models import db, ProtocolLog


def bookmark_helper(connection_id):

    log = ProtocolLog.query.filter_by(connection_id=str(connection_id)).first()
    log.bookmarked = True
    db.session.commit()

    return {"status": SUCCESS}


def unbookmark_helper(connection_id):
    log = ProtocolLog.query.filter_by(connection_id=str(connection_id)).first()
    log.bookmarked = False
    db.session.commit()

    return {"status": SUCCESS}
