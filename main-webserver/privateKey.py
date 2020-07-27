from app import *
from app.helpers.utils.general.sql_commands import *
from app.helpers.utils.azure.azure_resource_creation import *
from app.helpers.utils.general.tokens import *


output = fractalRunSQL(command="""
    SELECT *
    FROM v_ms WHERE private_key IS NULL;
""", params="")

for vm in output["rows"]:
    private_key = generatePrivateKey()
    fractalSQLUpdate(table_name="v_ms", conditional_params={"vm_name": vm["vm_name"]}, new_params={"private_key": private_key})
