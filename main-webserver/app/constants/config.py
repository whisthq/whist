import os
import sqlalchemy as db
from sqlalchemy.orm import sessionmaker

config_engine = db.create_engine(os.getenv("CONFIG_DB_URL"), echo=False, pool_pre_ping=True)
ConfigSession = sessionmaker(bind=config_engine, autocommit=False)

def getEnvVar(key):
    if os.getenv("USE_PRODUCTION_KEYS").upper() == "true":
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
        print("Error executing fetching config variable [{command}]: {error}".format(
            command=str(command), error=str(e)
        ))

    session.commit()
    session.close()
    return output[1]


DATABASE_URL = getEnvVar("DATABASE_URL")

VM_GROUP = (
    "Fractal"
    if os.getenv("USE_PRODUCTION_KEYS").upper() == "true"
    else "FractalStaging"
)

STRIPE_SECRET = getEnvVar("STRIPE_SECRET")

SILENCED_ENDPOINTS = ["/status", "/ping"]
