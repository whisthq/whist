from app.imports import *
from app.helpers.utils.general.logs import *

from app.models.public import *
from app.models.hardware import *
from app.models.devops import *
from app.models.logs import *
from app.models.sales import *


engine = db.create_engine(DATABASE_URL, echo=False, pool_pre_ping=True)
metadata.create_all(bind=engine)
Session = sessionmaker(bind=engine, autocommit=False)


def fractalCleanSQLOutput(output):
    if output:
        is_list = isinstance(output, list)
        if is_list:
            return [dict(row) for row in output]
        else:
            return dict(output)
    return None


def fractalRunSQL(command, params):
    session = Session()

    try:
        rows = session.execute(command, params)

        if rows.returns_rows:
            output = {
                "success": True,
                "returns_rows": True,
                "rows": fractalCleanSQLOutput(rows.fetchall()),
                "error": None,
            }
        else:
            output = {
                "success": True,
                "returns_rows": False,
                "rows": None,
                "error": None,
            }
    except Exception as e:
        output = {
            "success": False,
            "returns_rows": False,
            "rows": None,
            "error": str(e),
        }

        fractalLog(
            function="fractalRunSQL",
            label="None",
            logs="Error executing SQL command [{command}]: {error}".format(
                command=str(command), error=str(e)
            ),
            level=logging.ERROR,
        )

    session.commit()
    session.close()

    return output

def tableToObject(table_name):
    tableMap = {
        "users": User,
        "os_disks": OSDisk,
        "user_vms": UserVM,
        "secondary_disks": SecondaryDisk,
        "install_commands": InstallCommand,
        "apps_to_install": AppsToInstall,
        "release_groups": ReleaseGroup,
        "stripe_products": StripeProduct,
        "main_newsletter": MainNewsletter
    }


def fractalSQLSelect(table_name, params):
    session = Session()

    table_object = tableToObject(table_name)
    rows = session.query(table_object).filter_by(**params)

    session.commit()
    session.close()

    rows = rows.all()
    result = [row.__dict__ for row in rows]

    return result


def fractalSQLUpdate(table_name, conditional_params, new_params):
    """Updates an entry in the database

    Args:
        table_name (str): Name of table to update
        conditional_params (arr[str]): The params that need to be satisfied for the row to update
        new_params (arr[str]): The new values to update

    Returns:
        [type]: [description]
    """
    session = Session()

    table_object = tableToObject(table_name)
    session.query(table_object).filter_by(**conditional_params).update(new_params)

    session.commit()
    session.close()


def fractalSQLInsert(table_name, params):
    session = Session()

    table_object = tableToObject(table_name)
    new_row = table_object(**params)

    session.add(new_row)

    session.commit()
    session.close()



def fractalSQLDelete(table_name, params, and_or="AND"):
    session = Session()

    table_object = tableToObject(table_name)

    rows = session.query(table_object).filter_by(and_(**params)) if and_or == "AND" else session.query(table_object).filter_by(or_(**params))
    rows.delete()

    session.commit()
    session.close()