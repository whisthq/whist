#!/usr/bin/env python

import os
from heroku_config import config_vars

app_name = os.environ["HEROKU_APP_NAME"]

# Database url will be chosen based on the $HEROKU_APP_NAME env var
# .netrc must be configured with HEROKU credentials
print(config_vars(app_name)["DATABASE_URL"])
