import os

DATABASE_URL = (
    os.getenv("DATABASE_URL")
    if os.getenv("USE_PRODUCTION_DATABASE").upper() == "TRUE"
    else os.getenv("HEROKU_POSTGRESQL_ORANGE_URL")
)
