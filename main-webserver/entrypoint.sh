#!/bin/bash

echo Entrypoint for docker deployment
# Sets env
export $(cat .envdocker | xargs)
heroku config -s --app cube-celery-staging >> .env
export $(cat .env | xargs)
set FLASK_APP=run.py

flask run