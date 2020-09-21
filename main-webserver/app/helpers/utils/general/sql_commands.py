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
            if f:
                f(db, *args)
            db.session.commit()
            commit_successful = True
            break
        except exc.OperationalError as e:
            if attempts < 5:
                db.session.rollback()
                time.sleep(2 ** attempts)
            else:
                fractalLog(
                    function="fractalSQLCommit",
                    label="None",
                    logs="SQL commit failed after five retries.",
                    level=logging.CRITICAL,
                )
                return False

    if commit_successful:
        fractalLog(
            function="fractalSQLCommit",
            label="None",
            logs="SQL commit successful after {num_tries} tries".format(num_tries=attempts),
        )

    return commit_successful


def fractalSQLUpdate(db, obj, attributes):
    for k, v in attributes.items():
        setattr(obj, k, v)
