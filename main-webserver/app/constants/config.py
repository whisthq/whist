import os
import sqlalchemy

from sqlalchemy.orm import sessionmaker

config_engine = sqlalchemy.create_engine(os.getenv("CONFIG_DB_URL"), echo=False, pool_pre_ping=True)
ConfigSession = sessionmaker(bind=config_engine, autocommit=False)

app_to_env = {
    "dev-webserver": {"database": "DEV_DB_URL", "env": "dev"},
    "staging-webserver": {"database": "STAGING_DB_URL", "env": "staging"},
    "main-webserver": {"database": "PROD_DB_URL", "env": "production"},
}


def getEnvVar(key):
    output, env = None, None

    try:
        env = app_to_env[os.getenv("HEROKU_APP_NAME")]["env"]
    except:
        env = app_to_env["dev-webserver"]["env"]

    session = ConfigSession()

    command = """
        SELECT * FROM \"{table_name}\" WHERE "{param_name}" = '{param_value}'""".format(
        table_name=env, param_name="key", param_value=key
    )
    params = ""
    try:
        rows = session.execute(command, params)
        output = rows.fetchone()
        if output:
            output = output[1]
    except Exception as e:
        print(
            "Error executing fetching config variable [{command}]: {error}".format(
                command=str(command), error=str(e)
            )
        )

    session.commit()
    session.close()

    return output


try:
    DATABASE_URL = os.getenv(app_to_env[os.getenv("HEROKU_APP_NAME")]["database"])
except:
    DATABASE_URL = os.getenv(app_to_env["dev-webserver"]["database"])

# Stripe
STRIPE_SECRET = getEnvVar("STRIPE_SECRET")
HOURLY_PLAN_ID = getEnvVar("HOURLY_PLAN_ID")
MONTHLY_PLAN_ID = getEnvVar("MONTHLY_PLAN_ID")
UNLIMITED_PLAN_ID = getEnvVar("UNLIMITED_PLAN_ID")
ENDPOINT_SECRET = getEnvVar("ENDPOINT_SECRET")

# Auth
JWT_SECRET_KEY = getEnvVar("JWT_SECRET_KEY")
DASHBOARD_USERNAME = getEnvVar("DASHBOARD_USERNAME")
DASHBOARD_PASSWORD = getEnvVar("DASHBOARD_PASSWORD")
SHA_SECRET_KEY = getEnvVar("SHA_SECRET_KEY")

# AWS
AWS_ACCESS_KEY = getEnvVar("AWS_ACCESS_KEY")
AWS_SECRET_KEY = getEnvVar("AWS_SECRET_KEY")

# Sendgrid
SENDGRID_API_KEY = getEnvVar("SENDGRID_API_KEY")
SENDGRID_EMAIL = "noreply@tryfractal.com"
FRONTEND_URL = getEnvVar("FRONTEND_URL")

# Misc
SILENCED_ENDPOINTS = ["/status", "/ping"]
