from app.imports import *
from app import *


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

    session.commit()
    session.close()

    return output


def fractalSQLSelect(table_name, params):
    command = """
        SELECT * FROM {table_name} WHERE""".format(
        table_name=table_name
    )

    if not params:
        command = """
            SELECT * FROM {table_name}""".format(
            table_name=table_name
        )
    else:
        number_of_params = 1

        for param_name, param_value in params.items():
            if number_of_params == 1:
                command += """ "{param_name}" = :{param_name}""".format(
                    param_name=param_name
                )
            else:
                command += """ AND "{param_name}" = :{param_name}""".format(
                    param_name=param_name
                )

            number_of_params += 1

        command = text(command)

    return fractalRunSQL(command, params)


def fractalSQLUpdate(table_name, conditional_params, new_params):
    number_of_new_params = number_of_conditional_params = 1

    command = """
        UPDATE {table_name} SET""".format(
        table_name=table_name
    )

    for param_name, param_value in new_params.items():
        if number_of_new_params == 1:
            command += """ "{param_name}" = :{param_name}""".format(
                param_name=param_name
            )
        else:
            command += ""","{param_name}" = :{param_name}""".format(
                param_name=param_name
            )

        number_of_new_params += 1

    for param_name, param_value in conditional_params.items():
        if number_of_conditional_params == 1:
            command += """ WHERE "{param_name}" = :{param_name}""".format(
                param_name=param_name
            )
        else:
            command += """ AND "{param_name}" = :{param_name}""".format(
                param_name=param_name
            )

        number_of_conditional_params += 1

    command = text(command)
    conditional_params.update(new_params)

    return fractalRunSQL(command, conditional_params)
