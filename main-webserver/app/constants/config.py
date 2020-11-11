import os
import sqlalchemy

from sqlalchemy.orm import sessionmaker

config_engine = sqlalchemy.create_engine(os.getenv("CONFIG_DB_URL"), echo=False, pool_pre_ping=True)
ConfigSession = sessionmaker(bind=config_engine, autocommit=False)

app_to_db = {
    "dev-webserver": "DEV_DB_URL",
    "staging-webserver": "STAGING_DB_URL",
    "main-webserver": "PROD_DB_URL",
}


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


try:
    DATABASE_URL = os.getenv(app_to_db[os.getenv("HEROKU_APP_NAME")])
except:
    DATABASE_URL = os.getenv(app_to_db["dev-webserver"])

VM_GROUP = "Fractal" if os.getenv("USE_PRODUCTION_KEYS").upper() == "TRUE" else "FractalStaging"

STRIPE_SECRET = getEnvVar("STRIPE_SECRET")

JWT_SECRET_KEY = getEnvVar("JWT_SECRET_KEY")
AWS_ACCESS_KEY = getEnvVar("AWS_ACCESS_KEY")
AWS_SECRET_KEY = getEnvVar("AWS_SECRET_KEY")
ENDPOINT_SECRET = getEnvVar("ENDPOINT_SECRET")
DASHBOARD_USERNAME = getEnvVar("DASHBOARD_USERNAME")
DASHBOARD_PASSWORD = getEnvVar("DASHBOARD_PASSWORD")
SHA_SECRET_KEY = getEnvVar("SHA_SECRET_KEY")
SENDGRID_API_KEY = getEnvVar("SENDGRID_API_KEY")
FRONTEND_URL = getEnvVar("FRONTEND_URL")
SENDGRID_EMAIL = "noreply@tryfractal.com"

SILENCED_ENDPOINTS = ["/status", "/ping"]
