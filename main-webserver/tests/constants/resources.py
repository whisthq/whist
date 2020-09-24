import os

from dotenv import *

load_dotenv(find_dotenv())


RESOURCE_GROUP = "FractalStaging"
TEST_HEROKU_PR_NUMBER = os.getenv("TEST_HEROKU_PR_NUMBER")
MAIN_WEBSERVER = (
    "https://main-webserver-pr-" + TEST_HEROKU_PR_NUMBER + ".herokuapp.com"
    if TEST_HEROKU_PR_NUMBER
    else "https://main-webserver.herokuapp.com"
)

SERVER_URL = (
    "http://127.0.0.1:7730"
    if not os.getenv("CI") and not os.getenv("CI") == "true"
    else MAIN_WEBSERVER
))
