import sys
import os
import tempfile
import pytest
import requests
from dotenv import load_dotenv

load_dotenv()
SERVER_URL = "https://" + os.getenv("HEROKU_APP_NAME") + ".herokuapp.com"
