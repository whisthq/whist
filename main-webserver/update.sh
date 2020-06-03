#!/bin/bash

heroku git:remote -a cube-celery-vm && \
git add . && \
git commit -m "pushing to production" && \
git push heroku master:master && \
curl -X POST --data-urlencode "payload={\"channel\": \"#general\", \"username\": \"fractal-bot\", \"text\": \"VM-Webserver Updated in Production on Heroku\", \"icon_emoji\": \":ghost:\"}" https://hooks.slack.com/services/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
