#!/bin/bash
rndbytes=$(hexdump -n 4 -e '"%08X"' /dev/random)
gitcommit=$(git rev-parse --short HEAD)
version=$gitcommit-$rndbytes

cd build64

echo $version > version

aws s3 cp update.sh s3://arn:aws:s3:us-east-1:747391415460:accesspoint/fractal-cloud-setup/Linux/update.sh
aws s3 cp FractalServer s3://arn:aws:s3:us-east-1:747391415460:accesspoint/fractal-cloud-setup/Linux/FractalServer
aws s3 cp version s3://arn:aws:s3:us-east-1:747391415460:accesspoint/fractal-cloud-setup/Linux/version

# notify Slack
curl -X POST --data-urlencode "payload={\"channel\": \"#general\", \"username\": \"fractal-bot\", \"text\": \"Linux Server Auto-update Pushed to Production VMs\", \"icon_emoji\": \":fractal:\"}" https://hooks.slack.com/services/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

echo version
