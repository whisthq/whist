import os
from dotenv import load_dotenv

load_dotenv()

DATABASE_URL = (
    os.getenv("DATABASE_URL")
    if os.getenv("USE_PRODUCTION_DATABASE").upper() == "TRUE"
    else os.getenv("HEROKU_POSTGRESQL_ORANGE_URL")
)

RESOURCE_GROUP = "FractalStaging"

SERVER_URL = "https://main-webserver-staging4.herokuapp.com"
