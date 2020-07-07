from app import *
from app.helpers.utils.general.sql_commands import *
from app.helpers.utils.general.crypto import *


output = fractalRunSQL(command="""
    SELECT *
    FROM users WHERE password_token IS NULL;
""", params="")

for user in output["rows"]:
    # decrypt current password
    password = jwt.decode(user["password"], os.getenv('SECRET_KEY'))

    # hash plaintext password
    hashed_password = hash_value(password['pwd'])

    # save as password_token
    fractalSQLUpdate(table_name="users", conditional_params={"username": user["username"]}, new_params={"password_token": hashed_password})
