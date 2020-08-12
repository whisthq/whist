import os

from dotenv import *

load_dotenv(find_dotenv())


RESOURCE_GROUP = "FractalStaging"

SERVER_URL = (
    "http://127.0.0.1:7730"
    if not os.getenv("CI") and not os.getenv("CI") == "true"
    else "https://main-webserver-pr-"
    + os.getenv("TEST_HEROKU_PR_NUMBER")
    + ".herokuapp.com"
)
