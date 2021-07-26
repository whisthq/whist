import time

from sqlalchemy.exc import OperationalError

from app.helpers.utils.general.logs import fractal_logger


def fractal_sql_commit(db, func=None, *args):  # pylint: disable=keyword-arg-before-vararg
    attempts = 0
    commit_successful = False

    while attempts <= 5 and not commit_successful:
        attempts = attempts + 1
        try:
            if func:
                func(db, *args)
            db.session.commit()
            commit_successful = True
            break
        except OperationalError:
            if attempts < 5:
                db.session.rollback()
                time.sleep(2 ** attempts)
            else:
                fractal_logger.critical("SQL commit failed after five retries.")
                return False

    if commit_successful:
        fractal_logger.info(
            "SQL commit successful after {num_tries} tries".format(num_tries=attempts)
        )

    return commit_successful


def fractal_sql_update(db, obj, attributes):  # pylint: disable=unused-argument
    for k, v in attributes.items():
        setattr(obj, k, v)
