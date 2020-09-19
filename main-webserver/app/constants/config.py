import os
import sqlalchemy

from sqlalchemy.orm import sessionmaker

config_engine = sqlalchemy.create_engine(os.getenv("CONFIG_DB_URL"), echo=False, pool_pre_ping=True)
ConfigSession = sessionmaker(bind=config_engine, autocommit=False)


def getEnvVar(key):
    if os.getenv("USE_PRODUCTION_KEYS").upper() == "TRUE":
        env = "production"
    else:
        env = "staging"

    session = ConfigSession()

    command = """
        SELECT * FROM \"{table_name}\" WHERE "{param_name}" = '{param_value}'""".format(
        table_name=env, param_name="key", param_value=key
    )
    params = ""
    try:
        rows = session.execute(command, params)
        output = rows.fetchone()
    except Exception as e:
        print(
            "Error executing fetching config variable [{command}]: {error}".format(
                command=str(command), error=str(e)
            )
        )

    session.commit()
    session.close()

    return output[1]


DATABASE_URL = (
    os.getenv("PROD_DB_URL")
    if os.getenv("USE_PRODUCTION_KEYS").upper() == "TRUE"
    else os.getenv("STAGING_DB_URL")
)

VM_GROUP = "Fractal" if os.getenv("USE_PRODUCTION_KEYS").upper() == "TRUE" else "FractalStaging"

STRIPE_SECRET = getEnvVar("STRIPE_SECRET")

JWT_SECRET_KEY = getEnvVar("JWT_SECRET_KEY")
AWS_ACCESS_KEY = getEnvVar("AWS_ACCESS_KEY")
AWS_SECRET_KEY = getEnvVar("AWS_SECRET_KEY")
ENDPOINT_SECRET = getEnvVar("ENDPOINT_SECRET")
DASHBOARD_USERNAME = getEnvVar("DASHBOARD_USERNAME")
DASHBOARD_PASSWORD = getEnvVar("DASHBOARD_PASSWORD")
SHA_SECRET_KEY = getEnvVar("SHA_SECRET_KEY")
ADMIN_PASSWORD = getEnvVar("ADMIN_PASSWORD")
SENDGRID_API_KEY = getEnvVar("SENDGRID_API_KEY")
FRONTEND_URL = getEnvVar("FRONTEND_URL")
VM_PASSWORD = getEnvVar("VM_PASSWORD")

HOURLY_PLAN_ID = getEnvVar("HOURLY_PLAN_ID")
MONTHLY_PLAN_ID = getEnvVar("MONTHLY_PLAN_ID")
UNLIMITED_PLAN_ID = getEnvVar("UNLIMITED_PLAN_ID")
SMALLDISK_PLAN_ID = getEnvVar("SMALLDISK_PLAN_ID")
MEDIUMDISK_PLAN_ID = getEnvVar("MEDIUMDISK_PLAN_ID")

AZURE_SUBSCRIPTION_ID = getEnvVar("AZURE_SUBSCRIPTION_ID")
AZURE_CLIENT_ID = getEnvVar("AZURE_CLIENT_ID")
AZURE_CLIENT_SECRET = getEnvVar("AZURE_CLIENT_SECRET")
AZURE_TENANT_ID = getEnvVar("AZURE_TENANT_ID")

SILENCED_ENDPOINTS = ["/status", "/ping"]
