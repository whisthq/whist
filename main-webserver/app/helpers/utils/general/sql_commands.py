from app.imports import *


engine = db.create_engine(os.getenv("DATABASE_URL"), echo=False, pool_pre_ping=True)
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


def fractalSQLInsert(table_name, params, unique_keys=None):
    if unique_keys:
        output = fractalSQLSelect(table_name, unique_keys)
        if output["success"] and output["rows"]:
            output = {
                "success": False,
                "returns_rows": False,
                "rows": None,
                "error": "A row with column values {unique_keys} already exists in {table_name}".format(
                    unique_keys=str(unique_keys), table_name=table_name
                ),
            }

    columns = values = "("
    number_of_params = len(params.keys())
    current_param_number = 1

    for param_name, param_value in params.items():
        if current_param_number == number_of_params:
            columns += '"{param_name}")'.format(param_name=param_name)
            values += ":{param_name})".format(param_name=param_name)
        else:
            columns += '"{param_name}", '.format(param_name=param_name)
            values += ":{param_name}, ".format(param_name=param_name)

        current_param_number += 1

    command = """
        INSERT INTO {table_name}{columns} VALUES{values}""".format(
        table_name=table_name, columns=columns, values=values
    )

    command = text(command)
    return fractalRunSQL(command, params)


def fractalSQLDelete(table_name, params, and_or="AND"):
    conditions = ""
    number_of_params = len(params.keys())
    current_param = 1

    for param_name, param_value in params.items():
        conditions += '"{param_name}" = :{param_name}'.format(param_name=param_name)
        if not current_param == number_of_params:
            conditions += " {} ".format(and_or.upper())
        current_param += 1

    command = "DELETE FROM {table_name} WHERE {conditions}".format(
        table_name=table_name, conditions=conditions
    )

    command = text(command)
    return fractalRunSQL(command, params)
