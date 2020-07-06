#/bin/bash
# Named arguments are optional see defaults below
# usage:
# ./deploy_artifact <vm_name> <run_id> --resource_group <azure resource group> \
# --artifact_name <artifact name> --web_server <web server>
artifact_name=${artifact_name:-windows_server_build}
resource_group=${resource_group:-FractalProtocolCI}
web_server=${web_server:-https://cube-celery-staging4.herokuapp.com/}
# get named params
while [ $# -gt 0 ]; do
   if [[ $1 == *"--"* ]]; then
        param="${1/--/}"
        echo param
   fi
  shift
done