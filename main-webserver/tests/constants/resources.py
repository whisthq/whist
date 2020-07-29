import os
from dotenv import load_dotenv

load_dotenv()

# DATABASE_URL = (
#     os.getenv("DATABASE_URL")
#     if os.getenv("USE_PRODUCTION_DATABASE").upper() == "TRUE"
#     else os.getenv("HEROKU_POSTGRESQL_ORANGE_URL")
# )

DATABASE_URL = "postgres://u97uj1m5q16qjm:pde86ce23ddf2bfa972db8c5d09e12968022c048011d97b5acd4f9fb3a2dda891@ec2-52-205-72-163.compute-1.amazonaws.com:5432/d4lf18ud6qj6nr"

RESOURCE_GROUP = "Fractal"

# SERVER_URL = "https://main-webserver-staging4.herokuapp.com"
SERVER_URL = "http://127.0.0.1:7730"
