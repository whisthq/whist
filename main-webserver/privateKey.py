from app import *
from app.helpers.utils.general.sql_commands import *
from app.helpers.utils.general.crypto import *


output = fractalRunSQL(
    command="""
    SELECT *
    FROM v_ms WHERE rsa_private_key IS NULL;
""",
    params="",
)

for vm in output["rows"]:
    private_key = genPrivateKey()
    fractalSQLUpdate(
        table_name="v_ms",
        conditional_params={"vm_name": vm["vm_name"]},
        new_params={"rsa_private_key": private_key},
    )
