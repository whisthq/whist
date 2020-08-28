from app.imports import *
from app.constants.config import *
from app.helpers.utils.general.logs import *

from sqlalchemy import exc


def fractalSQLCommit(db, f=None, *args):
    attempts = 0
    commit_successful = False

    while attempts <= 5 and not commit_successful:
        attempts = attempts + 1
        try:
            fractalLog(function="", label="", logs="Attempting SQL commit.")
            if f:
                f(db, *args)
            db.session.commit()
            commit_successful = True
            break
        except exc.OperationalError as e:
            fractalLog(function="", label="", logs=str(e))
            if attempts < 5:
                fractalLog(
                    function="",
                    label="",
                    logs="SQL commit failed. Attempting to roll back.",
                )
                db.session.rollback()
                time.sleep(2 ** attempts)
            else:
                logger.error("Maximum number of retries reached. Raising an error")
                return False

    return commit_successful
