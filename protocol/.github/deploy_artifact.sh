#/bin/bash
# Named arguments are optional see defaults below
# usage:
# ./deploy_artifact <vm_name> <run_id> --resource_group <azure resource group> \
# --artifact_name <artifact name> --web_server <web server>
vm_name=$1
run_id=$2

artifact_name=${artifact_name:-windows_server_build}
resource_group=${resource_group:-FractalProtocolCI}
web_server=${web_server:-https://cube-celery-staging4.herokuapp.com/}

# get named params
while [ $# -gt 0 ]; do
   if [[ $1 == *"--"* ]]; then
        param="${1/--/}"
        declare $param="$2"
   fi
  shift
done

echo "sending deploy post request"
curl --location --request POST ''$web_server'/artifact/deploy' \
--header 'Content-Type: application/json' \
--data-raw '{
    "vm_name": "'$vm_name'",
    "artifact_name": "'$artifact_name'",
    "run_id": "'$run_id'",
    "resource_group": "'$resource_group'"
}'
echo "Deploy command sent, you may need to wait a minute or so for the deployment to complete"



